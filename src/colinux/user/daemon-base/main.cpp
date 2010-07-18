#include <stdio.h>

#include "main.h"

user_daemon_t::user_daemon_t()
{
	co_rc_t rc;

	reactor = 0;
	monitor_handle = 0;
	param_index = 0;
	param_instance = 0;

	rc = co_reactor_create(&reactor);
	if (!CO_OK(rc)) {
		throw user_daemon_exception_t(rc);
	}
}

void user_daemon_t::handle_parameters(int argc, char *argv[])
{
	bool_t instance_specified, unit_specified;
	co_command_line_params_t cmdline;
	co_rc_t rc;

	rc = co_cmdline_params_alloc(&argv[1], argc-1, &cmdline);
	if (!CO_OK(rc)) {
		log("error parsing arguments\n");
		throw user_daemon_exception_t(rc);
	}

	try {

		rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-i",
								  &instance_specified,
								  &param_instance);

		if (!CO_OK(rc)) {
			syntax();
			throw user_daemon_exception_t(rc);
		}

		rc = co_cmdline_params_one_arugment_int_parameter(cmdline, "-u",
								  &unit_specified, &param_index);

		if (!CO_OK(rc)) {
			syntax();
			throw user_daemon_exception_t(rc);
		}

		handle_extended_parameters(cmdline);

		rc = co_cmdline_params_check_for_no_unparsed_parameters(cmdline, PTRUE);
		if (!CO_OK(rc)) {
			syntax();
			throw user_daemon_exception_t(rc);
		}

		if (!instance_specified  &&  !unit_specified) {
			rc = CO_RC(OK);
			syntax();
			throw user_daemon_exception_t(rc);
		}

		verify_parameters();

		if (param_index >= get_unit_count())
		{
			syntax();
			log("invalid unit index: %d\n", param_index);
			throw user_daemon_exception_t(rc);
		}

		if (!instance_specified) {
			syntax();
			log("coLinux instance not specificed\n");
			throw user_daemon_exception_t(rc);
		}

	} catch(...) {
		co_cmdline_params_free(cmdline);
		throw;
	}
}

void user_daemon_t::handle_extended_parameters(co_command_line_params_t cmdline)
{
}

void user_daemon_t::verify_parameters()
{

}

static user_daemon_t *user_daemon = 0;

co_rc_t monitor_receive(co_reactor_user_t user, unsigned char *buffer, unsigned long size)
{
	co_message_t *message;
	unsigned long message_size;
	long size_left = size;
	long position = 0;

	while (size_left > 0) {
		message = (typeof(message))(&buffer[position]);
		message_size = message->size + sizeof(*message);
		size_left -= message_size;
		if (size_left >= 0) {
			user_daemon->received_from_monitor(message);
		}
		position += message_size;
	}

	return CO_RC(OK);
}

void user_daemon_t::run(int argc, char *argv[])
{
	co_module_t modules[1];
	co_rc_t rc;

	handle_parameters(argc, argv);

	prepare_for_loop();

	modules[0] = (co_module_t)(get_base_module() + param_index);

	rc = co_user_monitor_open(reactor, monitor_receive,
				  param_instance, modules,
				  sizeof(modules)/sizeof(co_module_t),
				  &monitor_handle);
	if (!CO_OK(rc)) {
		log("cannot connect to monitor\n");
		return;
	}

	user_daemon = this;

	while (1) {
		co_rc_t rc;
		rc = co_reactor_select(reactor, 10);
		if (!CO_OK(rc))
			break;
	}
}

void user_daemon_t::send_to_monitor(co_message_t *message)
{
	if (!monitor_handle)
		return;

	monitor_handle->reactor_user->send(monitor_handle->reactor_user, (unsigned char *)message,
					   message->size + sizeof(*message));
}

void user_daemon_t::send_to_monitor_raw(co_device_t device, unsigned char *buffer, unsigned long size)
{
	struct {
		co_message_t message;
		co_linux_message_t msg_linux;
		char data[];
	} *message;

	message = (typeof(message))co_os_malloc(sizeof(*message) + size);
	if (!message)
		return;

	message->message.from = (co_module_t)(get_base_module() + param_index);
	message->message.to = CO_MODULE_LINUX;
	message->message.priority = CO_PRIORITY_DISCARDABLE;
	message->message.type = CO_MESSAGE_TYPE_OTHER;
	message->message.size = sizeof(message->msg_linux) + size;
	message->msg_linux.device = device;
	message->msg_linux.unit = (int)param_index;
	message->msg_linux.size = size;
	co_memcpy(message->data, buffer, size);

	send_to_monitor(&message->message);

	co_os_free(message);
}

void user_daemon_t::prepare_for_loop()
{
}

void user_daemon_t::syntax()
{
	co_terminal_print("Cooperative Linux %s\n", get_daemon_title());
	co_terminal_print("Dan Aloni, 2004 (c)\n");
	co_terminal_print("\n");
	co_terminal_print("syntax: \n");
	co_terminal_print("\n");
	co_terminal_print("  %s -i pid -u unit_index %s\n", get_daemon_name(), get_extended_syntax());
	co_terminal_print("\n");
	co_terminal_print("    -i pid              coLinux instance ID to connect to\n");
	co_terminal_print("    -u unit_index       Network device index number (e.g, 0 for eth0, 1 for\n");
	co_terminal_print("                        eth1, etc.)\n");
}

const char *user_daemon_t::get_extended_syntax()
{
	return "";
}

void user_daemon_t::log(const char *format, ...)
{
	char buf[0x100];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buf, sizeof(buf), format, ap);
	va_end(ap);

	co_terminal_print("%s: %s", get_daemon_name(), buf);
}

user_daemon_t::~user_daemon_t()
{
	co_reactor_destroy(reactor);
}
