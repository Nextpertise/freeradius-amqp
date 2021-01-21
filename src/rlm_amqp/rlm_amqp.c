/*
 *   This program is is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or (at
 *   your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/**
 * $Id$
 * @file rlm_amqp.c
 * @brief amqp module code.
 *
 * @copyright 2013 The FreeRADIUS server project
 * @copyright 2021 your name \<your address\>
 */
RCSID("$Id$")

#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/modules.h>
#include <freeradius-devel/rad_assert.h>

#include <json-c/json.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include "utils.h"

#include <sys/time.h>

/*
 *	Define a structure for our module configuration.
 *
 *	These variables do not need to be in a structure, but it's
 *	a lot cleaner to do so, and a pointer to the structure can
 *	be used as the instance handle.
 */
typedef struct rlm_amqp_t {
	char const *hostname;
	uint32_t port;
	char const *username;
	char const *password;
	char const *exchange;
	char const *routingkey;
	uint32_t connect_timout;
	char const *auth_data;
	char const *acct_data;
	char const *custom_key_hostname;
	char const *custom_key_key2;
	char const *missed_file;
} rlm_amqp_t;

/*
 *	A mapping of configuration file names to internal variables.
 */
static const CONF_PARSER module_config[] = {
		{ "hostname", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, hostname),"localhost" },
		{ "port", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_amqp_t, port), "5672" },
		{ "username", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, username), "guest" },
		{ "password",FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, password), "guest" },
		{ "exchange", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, exchange),"amq.direct" },
		{ "routingkey", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, routingkey), "rlm_amqp" },
		{ "connect_timout", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_amqp_t, connect_timout), "5" },
		{ "auth_data",FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, auth_data), NULL },
		{ "acct_data", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t,acct_data), NULL },
		{ "custom_key_hostname",FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, custom_key_hostname),NULL },
		{ "custom_key_key2", FR_CONF_OFFSET(PW_TYPE_STRING,rlm_amqp_t, custom_key_key2), NULL },
		{ "missed_file", FR_CONF_OFFSET(PW_TYPE_STRING,rlm_amqp_t, missed_file), "/tmp/amqp_missed.txt" },
		CONF_PARSER_TERMINATOR
};

amqp_socket_t *amqp_socket = NULL;
amqp_connection_state_t conn;

static void disconnect_amqp(void){

	DEBUG("Closing AMQP Connection");

	die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS),
					"Closing channel");
	die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS),
					"Closing connection");
	die_on_error(amqp_destroy_connection(conn), "Ending connection");

}

static int connect_amqp(void *instance) {

	rlm_amqp_t *inst = instance;

	DEBUG("Connecting to AMQP");

	if(amqp_socket){
		amqp_socket = NULL;
		die_on_error(amqp_destroy_connection(conn), "Ending connection");

	}

	conn = amqp_new_connection();
	amqp_socket = amqp_tcp_socket_new(conn);

	if (!amqp_socket) {
		ERROR("Unable to create AMQP socket: %d", inst->port);
		return -1;
	}

	int status = 1;
	struct timeval tval;
	struct timeval *tv;
	tv = &tval;
	tv->tv_sec = inst->connect_timout;
	status = amqp_socket_open_noblock(amqp_socket, inst->hostname, inst->port, tv); //(socket, inst->hostname, inst->port);
	if (status) {
		ERROR("Error opening TCP socket: %d, status: %d",inst->port, status);
		return -1;
	}

	die_on_amqp_error(
				amqp_login(conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
						inst->username, inst->password), "Logging in");
	amqp_channel_open(conn, 1);
	die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");

	return 0;
}

static void dump_to_file(void *instance, char const *content){
	FILE *fptr;
	rlm_amqp_t *inst = instance;
	fptr = fopen(inst->missed_file,"a");
	if(fptr == NULL) {
		ERROR("Error in file operation, messge will not be dumped: %s",content);
		return;
	}
	DEBUG("Dumping mayload to missed_file: %s",inst->missed_file);
	fprintf(fptr,"%s\n", content);
	fclose(fptr);
}

static int put_amqp(void *instance, char const *messagebody) {

	rlm_amqp_t *inst = instance;

	if(!(amqp_get_sockfd(conn) >= 0)){
		DEBUG("Socket is closed, reconnecting");
		connect_amqp(instance);
	}

	{
		amqp_basic_properties_t props;
		props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG
				| AMQP_BASIC_DELIVERY_MODE_FLAG;
		props.content_type = amqp_cstring_bytes("text/plain");
		props.delivery_mode = 2; /* persistent delivery mode */
		int ret = die_on_error(
				amqp_basic_publish(conn, 1, amqp_cstring_bytes(inst->exchange),
						amqp_cstring_bytes(inst->routingkey), 0, 0, &props,
						amqp_cstring_bytes(messagebody)), "Publishing");
		if(!ret){
			ERROR("Error publishing to AMQP, connection will be re-attempted");
			dump_to_file(instance, messagebody);
			connect_amqp(instance);
		} else {
			DEBUG("Publish successfull, payload: %s",messagebody);
		}
	}

	return 0;

}


static void handle_amqp(void *instance, REQUEST *request, char const *evt) {
	rlm_amqp_t *inst = instance;

	DEBUG("Sending data to AMQP event: %s, host: %s, port: %d, user: %s, exchange: %s", evt, inst->hostname, inst->port, inst->username, inst->exchange);

	char *copy;
	if (strcmp(evt, "authenticate") == 0) {
		copy = strdup(inst->auth_data);
	} else if (strcmp(evt, "preacct") == 0) {
		copy = strdup(inst->acct_data);
	} else if (strcmp(evt, "accounting") == 0) {
		copy = strdup(inst->acct_data);
	}

	char *token = strtok(copy, ",");

	VALUE_PAIR *item_vp;
	json_object *json = json_object_new_object();
	json_object_object_add(json, "event_type", json_object_new_string(evt));
	json_object_object_add(json, "custom_key_hostname",
			json_object_new_string(inst->custom_key_hostname));
	json_object_object_add(json, "custom_key_key2",
			json_object_new_string(inst->custom_key_key2));

	while (token != NULL) {

		item_vp = request->packet->vps;
		int found = 0;
		while (item_vp != NULL) {

			if (strcmp(item_vp->da->name, token) == 0) {

				if (item_vp->da->type == PW_TYPE_STRING && item_vp->vp_strvalue) {
					json_object_object_add(json, token,
							json_object_new_string(item_vp->vp_strvalue));
				} else if (item_vp->da->type == PW_TYPE_INTEGER) {
					json_object_object_add(json, token,
							json_object_new_int(item_vp->vp_integer));
				}

				found = 1;
			}
			if (!found) {
				json_object_object_add(json, token, NULL);
			}
			item_vp = item_vp->next;
		}

		token = strtok(NULL, ",");
	}

	put_amqp(instance, json_object_to_json_string(json));
	free(copy);

}

/*
 *	Do any per-module initialization that is separate to each
 *	configured instance of the module.  e.g. set up connections
 *	to external databases, read configuration files, set up
 *	dictionary entries, etc.
 */
static int mod_instantiate(CONF_SECTION *conf, void *instance) {
	rlm_amqp_t *inst = instance;
	ATTR_FLAGS flags;

	memset(&flags, 0, sizeof(flags));

	if (inst->auth_data[0] == '\0') {
		cf_log_err_cs(conf, "auth_data is null: forcing error!");
		return -1;
	}

	if (inst->acct_data[0] == '\0') {
		cf_log_err_cs(conf, "acct_data is null: forcing error!");
		return -1;
	}

	connect_amqp(instance);

	return 0;
}

/*
 *	Find the named user in this modules database.  Create the set
 *	of attribute-value pairs to check and reply with for this user
 *	from the database. The authentication code only needs to check
 *	the password, the rest is done here.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_authorize(UNUSED void *instance,
		REQUEST *request) {

	handle_amqp(instance, request, "authenticate");

	return RLM_MODULE_OK;
}

/*
 *	Authenticate the user with the given password.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_authenticate(UNUSED void *instance,
		UNUSED REQUEST *request) {
	return RLM_MODULE_OK;
}

#ifdef WITH_ACCOUNTING
/*
 *	Massage the request before recording it or proxying it
 */
static rlm_rcode_t CC_HINT(nonnull) mod_preacct(UNUSED void *instance,
		UNUSED REQUEST *request) {
	handle_amqp(instance, request, "preacct");
	return RLM_MODULE_OK;
}

/*
 *	Write accounting information to this modules database.
 */
static rlm_rcode_t CC_HINT(nonnull) mod_accounting(UNUSED void *instance,
		UNUSED REQUEST *request) {
	handle_amqp(instance, request, "accounting");

	return RLM_MODULE_OK;
}

/*
 *	See if a user is already logged in. Sets request->simul_count to the
 *	current session count for this user and sets request->simul_mpp to 2
 *	if it looks like a multilink attempt based on the requested IP
 *	address, otherwise leaves request->simul_mpp alone.
 *
 *	Check twice. If on the first pass the user exceeds his
 *	max. number of logins, do a second pass and validate all
 *	logins by querying the terminal server (using eg. SNMP).
 */
static rlm_rcode_t CC_HINT(nonnull) mod_checksimul(UNUSED void *instance,
		REQUEST *request) {
	request->simul_count = 0;

	return RLM_MODULE_OK;
}
#endif

/*
 *	Only free memory we allocated.  The strings allocated via
 *	cf_section_parse() do not need to be freed.
 */
static int mod_detach(UNUSED void *instance) {
	/* free things here */
	disconnect_amqp();
	return 0;
}

/*
 *	The module name should be the only globally exported symbol.
 *	That is, everything else should be 'static'.
 *
 *	If the module needs to temporarily modify it's instantiation
 *	data, the type should be changed to RLM_TYPE_THREAD_UNSAFE.
 *	The server will then take care of ensuring that the module
 *	is single-threaded.
 */
extern module_t rlm_amqp;
module_t rlm_amqp = {
		.magic = RLM_MODULE_INIT,
		.name = "amqp",
		.type = RLM_TYPE_THREAD_SAFE,
		.inst_size = sizeof(rlm_amqp_t),
		.config = module_config,
		.instantiate = mod_instantiate,
		.detach = mod_detach,
		.methods = {
				[MOD_AUTHENTICATE] = mod_authenticate,
				[MOD_AUTHORIZE] = mod_authorize,
#ifdef WITH_ACCOUNTING
				[MOD_PREACCT] = mod_preacct,
				[MOD_ACCOUNTING] = mod_accounting,
				[MOD_SESSION] = mod_checksimul
#endif
				},
};
