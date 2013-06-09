#include <glib.h>
#include <gio/gio.h>
#include <agent.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"
#include "callbacks.h"
#include "global.h"

gboolean
exchange_credentials(NiceAgent *agent, guint stream_id, gpointer data) {
  g_debug("exchange_credentials(): candidate gathering done\n");

  publish_local_credentials(agent, stream_id);
  lookup_remote_credentials(agent, stream_id);

  g_debug("candidate gathering done\n");
}

void
start_server(NiceAgent *agent, guint stream_id, guint component_id, guint state, gpointer server_ptr) {
  GSocketService* server = (GSocketService*) server_ptr;
  g_debug("Server starts listening.\n");
  g_socket_service_start(server);
}

void
start_server_reliable(NiceAgent *agent, guint stream_id, guint component_id, gpointer server_ptr) {
  GSocketService* server = (GSocketService*) server_ptr;
  g_debug("Server starts listening.\n");
  g_socket_service_start(server);
}

void
attach_stdin2send_callback(NiceAgent *agent, guint stream_id, guint component_id, guint state) {
  if (state == NICE_COMPONENT_STATE_READY) {
    unpublish_local_credentials(agent, stream_id);
    GIOChannel* io_stdin;
    io_stdin = g_io_channel_unix_new(fileno(stdin));

    g_io_add_watch(io_stdin, G_IO_IN, send_data, agent);
  }

  if (state == NICE_COMPONENT_STATE_FAILED) {
    g_main_loop_quit (gloop);
  }
}

void
attach_stdin2send_callback_reliable(NiceAgent *agent, guint stream_id, guint component_id, gpointer data) {
  unpublish_local_credentials(agent, stream_id);

  GIOChannel* io_stdin;
  io_stdin = g_io_channel_unix_new(fileno(stdin));

  g_io_add_watch(io_stdin, G_IO_IN, send_data, agent);
}

gboolean
send_data(GIOChannel *source, GIOCondition cond, gpointer agent_ptr) {
  g_debug("send_data()\n");

  NiceAgent *agent = agent_ptr;
  gsize max_len = 10240;
  gchar *data = g_malloc(max_len);
  gsize len;

  gint total = 0;
  do {
    if(g_io_channel_read_chars(source, data, max_len, &len, NULL) == G_IO_STATUS_NORMAL)
      total += nice_agent_send(agent, nice_stream_id, 1, len, data);
    else
      break;
  }
  while(len < max_len);

  g_debug("send_data(): sent %u bytes\n", total);
  g_free(data);
  return TRUE;
}

void
recv_data2fd(NiceAgent *agent, guint stream_id, guint component_id, guint len,
    gchar *buf, gpointer data) {
  g_debug("recv_data2fd(fd=%u, len=%u)\n", output_fd, len);
  write(output_fd, buf, len);
  syncfs(output_fd);
}
