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
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/tcp_socket.h>
#include "utils.h"

#include <sys/time.h>

#define type_name(expr) \ 
    (_Generic((expr), \ 
              char: "char", unsigned char: "unsigned char", signed char: "signed char", \ 
              short: "short", unsigned short: "unsigned short", \ 
              int: "int", unsigned int: "unsigned int", \ 
              long: "long", unsigned long: "unsigned long", \ 
              long long: "long long", unsigned long long: "unsigned long long", \ 
              float: "float", \ 
              double: "double", \ 
              long double: "long double", \ 
              void*: "void*", \ 
              default: "?")) 
	

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
	char const *vhost;
	char const *routingkey;
	uint32_t connect_timeout;
	char const *auth_data;
	char const *acct_data;
	char const *custom_kvp;
	char const *missed_file;
	amqp_socket_t *amqp_socket;
	amqp_connection_state_t conn;
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
		{ "vhost", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, vhost),"/" },
		{ "routingkey", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, routingkey), "rlm_amqp" },
		{ "connect_timeout", FR_CONF_OFFSET(PW_TYPE_INTEGER, rlm_amqp_t, connect_timeout), "5" },
		{ "auth_data",FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, auth_data), NULL },
		{ "acct_data", FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t,acct_data), NULL },
		{ "custom_kvp",FR_CONF_OFFSET(PW_TYPE_STRING, rlm_amqp_t, custom_kvp),"" },
		{ "missed_file", FR_CONF_OFFSET(PW_TYPE_STRING,rlm_amqp_t, missed_file), "/tmp/amqp_missed.txt" },
		CONF_PARSER_TERMINATOR
};


static void disconnect_amqp(void *instance){
	rlm_amqp_t *inst = instance;
	DEBUG("Closing AMQP Connection");

	die_on_amqp_error(amqp_channel_close(inst->conn, 1, AMQP_REPLY_SUCCESS),
					"Closing channel");
	die_on_amqp_error(amqp_connection_close(inst->conn, AMQP_REPLY_SUCCESS),
					"Closing connection");
	die_on_error(amqp_destroy_connection(inst->conn), "Ending connection");

}



static int connect_amqp(void *instance) {

	rlm_amqp_t *inst = instance;

	DEBUG("Connecting to AMQP");

	if(inst->amqp_socket){
		inst->amqp_socket = NULL;
		die_on_error(amqp_destroy_connection(inst->conn), "Ending connection");

	}
	struct timeval tval;
	struct timeval *tv;
	tv = &tval;
	tv->tv_sec = inst->connect_timeout;

	inst->conn = amqp_new_connection();
	amqp_set_handshake_timeout(inst->conn, tv);

	inst->amqp_socket = amqp_tcp_socket_new(inst->conn);

	if (!inst->amqp_socket) {
		ERROR("Unable to create AMQP socket: %d", inst->port);
		return -1;
	}

	int status = 1;

	status = amqp_socket_open_noblock(inst->amqp_socket, inst->hostname, inst->port, tv); //(socket, inst->hostname, inst->port);
//	status = amqp_socket_open(inst->amqp_socket, inst->hostname, inst->port);
	if (status) {
		ERROR("Error opening TCP socket: %d, status: %d",inst->port, status);
		return -1;
	}

	int logon = 0;
	logon = die_on_amqp_error(
				amqp_login(inst->conn, inst->vhost, 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
						inst->username, inst->password), "Logging in");
	if(!logon) {
		ERROR("Logon not successfull");
		return -1;
	}
	amqp_channel_open(inst->conn, 1);
	die_on_amqp_error(amqp_get_rpc_reply(inst->conn), "Opening channel");

        DEBUG("Connected to AMQP");

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
	DEBUG("Dumping payload to missed_file: %s",inst->missed_file);
	fprintf(fptr,"%s\n", content);
	fclose(fptr);
}


static int put_amqp(void *instance, char const *messagebody) {

	rlm_amqp_t *inst = instance;

	if(!(amqp_get_sockfd(inst->conn) >= 0)){
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
				amqp_basic_publish(inst->conn, 1, amqp_cstring_bytes(inst->exchange),
						amqp_cstring_bytes(inst->routingkey), 1, 0, &props,
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

static const char* get_reply_text(int code){
	switch(code){
	case 1:
		return "Access-Request";
	case 2:
		return "Access-Accept";
	case 3:
		return "Access-Reject";
	case 4:
		return "Accounting-Request";
	case 5:
		return "Accounting-Response";
	case 11:
		return "Access-Challange";
	case 12:
		return "Status-Server";
	case 13:
		return "Status-Client";
	default:
		return "Unknown";
	}
}


static void handle_amqp(void *instance, REQUEST *request, char const *evt) {
	rlm_amqp_t *inst = instance;

	RDEBUG("Sending data to AMQP event: %s, host: %s, port: %d, user: %s, exchange: %s",evt, inst->hostname, inst->port, inst->username, inst->exchange);


	VALUE_PAIR *item_vp;
	json_object *json = json_object_new_object();
	json_object_object_add(json, "event_type", json_object_new_string(evt));

	char *ckvp = strdup(inst->custom_kvp);
	char *kvp = strtok(ckvp, ",");

	while(kvp != NULL){
		char *key = malloc(100);
		char *val = malloc(100);
		if(sscanf(kvp, "%[^=]=%s", key, val) == 2) {
			json_object_object_add(json, key, json_object_new_string(val));
		} else {
			RERROR("Error in setting custom_kvp: %s", kvp);
		}

		kvp = strtok(NULL, " ");

	}


	//json_object_object_add(json, "reply_code", json_object_new_int(request->reply->code));
	json_object_object_add(json, "reply_code", json_object_new_string(get_reply_text(request->reply->code)));

	json_object *data = json_object_new_object();
	char *copy;
		if (strcmp(evt, "authenticate") == 0) {
			copy = strdup(inst->auth_data);
		} else if (strcmp(evt, "preacct") == 0) {
			copy = strdup(inst->acct_data);
		} else if (strcmp(evt, "accounting") == 0) {
			copy = strdup(inst->acct_data);
		} else {
			copy = strdup(inst->auth_data);
		}
	char *token = strtok(copy, ",");
	while (token != NULL) {

		item_vp = request->packet->vps;
		int found = 0;
		while (item_vp != NULL) {

			if (strcmp(item_vp->da->name, token) == 0) {

				if (item_vp->da->type == PW_TYPE_STRING && item_vp->vp_strvalue) {
					json_object_object_add(data, token,
							json_object_new_string(item_vp->vp_strvalue));
				} else if (item_vp->da->type == PW_TYPE_INTEGER) {
					json_object_object_add(data, token,
							json_object_new_int(item_vp->vp_integer));
				} else if (item_vp->da->type == PW_TYPE_IPV4_ADDR) {
					printf("%s is of type %s\n", item_vp->vp_ipaddr, type_name(item_vp->vp_ipaddr));
					printf("%d is of type %s\n", item_vp->vp_ipaddr, type_name(item_vp->vp_ipaddr));
					json_object_object_add(data, token,
							json_object_new_int(item_vp->vp_ipaddr));
				}

				found = 1;
			}
			if (!found) {
				json_object_object_add(data, token, NULL);
			}
			item_vp = item_vp->next;
		}

		token = strtok(NULL, ",");
	}
	json_object_object_add(json, "data", data);
	put_amqp(instance, json_object_to_json_string(json));
	free(copy);
	free(ckvp);
	free(kvp);
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

	sleep(5);
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
	handle_amqp(instance, request, "authenticate");
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

static rlm_rcode_t CC_HINT(nonnull) mod_post_auth(void *instance, REQUEST *request) {
		handle_amqp(instance, request, "post-auth");

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
	disconnect_amqp(instance);
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
				[MOD_POST_AUTH]	= mod_post_auth,
#ifdef WITH_ACCOUNTING
				[MOD_PREACCT] = mod_preacct,
				[MOD_ACCOUNTING] = mod_accounting,
				[MOD_SESSION] = mod_checksimul
#endif
				},
};
