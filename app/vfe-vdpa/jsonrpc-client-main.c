#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>

#include "jsonrpc-client.h"
#include "vdpa_rpc.h"

/* Used by main to communicate with parse_opt */
struct arguments {
	char *serverip;
	char *add;
	char *remove;
	char *list;
	char *info;
	char *vhost_socket;
	char *vm_uuid;
};

#define LOCAL_HOST "127.0.0.1"
#define PARAM_SIZE 512

/* Program documentation */
static char doc[] = "vfe-vhostd command-line C client for VF operations";
static char args_doc[] = "";

/* Command-line options */
static struct argp_option options[] = {
	{"serverip", 's', "SERVERIP", 0, "server ip to connect to"},
	{"add", 'a', "DEVICE", 0, "add a pci device"},
	{"remove", 'r', "DEVICE", 0, "remove a pci device"},
	{"list", 'l', "DEVICE", 0, "list all VF devices of PF device"},
	{"info", 'i', "DEVICE", 0, "show specified VF device information"},
	{"vhost_socket", 'v', "FILE", 0, "Vhost socket file name"},
	{"vm_uuid", 'u', "UUID", 0, "Virtual machine UUID"},
	{0}
};

/* Parse a single option */
static error_t
parse_opt(int key, char *arg, struct argp_state *state) {
	struct arguments *arguments = state->input;

	switch (key) {
		case 's':
			arguments->serverip = arg;
			break;
		case 'a':
			arguments->add = arg;
			break;
		case 'r':
			arguments->remove = arg;
			break;
		case 'l':
			arguments->list = arg;
			break;
		case 'i':
			arguments->info = arg;
			break;
		case 'v':
			arguments->vhost_socket = arg;
			break;
		case 'u':
			arguments->vm_uuid = arg;
			break;
		case ARGP_KEY_ARG:
			break;
		case ARGP_KEY_END:
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

/* Argp parser configuration */
static struct argp argp = {options, parse_opt, args_doc, doc};

static int
gen_add_params(char *params, int psize, struct arguments *args)
{
	int ret = 0;
	if (!args->vhost_socket) {
		fprintf(stderr, "Please provide vhost socket for add\n");
		return -1;
	}
	if (args->vm_uuid) {
		ret = snprintf(params, psize,
				"{\"add\":true,\"vfdev\":\"%s\",\"socket_file\":\"%s\",\"uuid\":\"%s\"}",
				args->add, args->vhost_socket, args->vm_uuid);
	} else {
		ret = snprintf(params, psize,
				"{\"add\":true,\"vfdev\":\"%s\",\"socket_file\":\"%s\"}",
				args->add, args->vhost_socket);
	}
	return ret;
}

int
main(int argc, char *argv[])
{
	struct arguments args;
	struct jsonrpc_client client = {
		.ip = LOCAL_HOST,
		.port = VDPA_RPC_PORT,
	};
	char params[PARAM_SIZE];
	int ret = 0;

	memset(&args, 0, sizeof(struct arguments));
	memset(params, 0, sizeof(params));

	/* Parse arguments */
	argp_parse(&argp, argc, argv, 0, 0, &args);

	if (args.serverip)
		client.ip = args.serverip;

	if (args.add) {
		ret = gen_add_params(params, sizeof(params), &args);
		if (ret < 0) {
			fprintf(stderr, "Generate parameters for add failed\n");
			return 1;
		}
	} else if (args.remove) {
		snprintf(params, PARAM_SIZE, "{\"remove\":true,\"vfdev\":\"%s\"}", args.remove);
	} else if (args.list) {
		snprintf(params, PARAM_SIZE, "{\"list\":true,\"mgmtpf\":\"%s\"}", args.list);
	} else if (args.info) {
		snprintf(params, PARAM_SIZE, "{\"info\":true,\"vfdev\":\"%s\"}", args.info);
	}

	if (params[0] != '\0') {
		cJSON *result = jsonrpc_client_call_method(&client, "vf", params);
		if (!result) {
			fprintf(stderr, "Can't get valid response\n");
			return 2;
		}
	} else {
		fprintf(stderr, "No valid command\n");
		return 3;
	}

	return 0;
}
