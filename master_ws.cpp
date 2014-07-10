/*
 * This is a hack of the test-echo program from the libwebsockets distro.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <assert.h>
#include <signal.h>

#include <syslog.h>
#include <sys/time.h>
#include <unistd.h>

#include <libwebsockets.h>

#include "server_msg.pb.h"
#include "master_server.hpp"

static volatile int force_exit = 0;

// I think this is just for SSL certs
#define LOCAL_RESOURCE_PATH "./www"

#define max_payload 2048
struct per_session_data__bling_protobuf
{
   unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + max_payload + LWS_SEND_BUFFER_POST_PADDING];
   unsigned int len;
   unsigned int index;
};

// Global ugh!
Master_Server* ms = NULL;

void hexdump(const void *ptr, int buflen)
{
   printf("len:%d\n", buflen);
   unsigned char *buf = (unsigned char*)ptr;
   int i, j;
   for (i=0; i<buflen; i+=16) {
      printf("%06x: ", i);
      for (j=0; j<16; j++) 
         if (i+j < buflen)
            printf("%02x ", buf[i+j]);
         else
            printf("   ");
      printf(" ");
      for (j=0; j<16; j++) 
         if (i+j < buflen)
            printf("%c", isprint(buf[i+j]) ? buf[i+j] : '.');
      printf("\n");
   }
}

static int
callback_bling_protobuf(struct libwebsocket_context *context,
                        struct libwebsocket *wsi,
                        enum libwebsocket_callback_reasons reason, void *user,
                        void *in, size_t len)
{
   struct per_session_data__bling_protobuf *pss = (struct per_session_data__bling_protobuf *)user;
   int n;
   if (ms == NULL)
   {
      lwsl_err("master server object no initialized\n");
      return 1;
   }  

   switch (reason)
   {
      case LWS_CALLBACK_SERVER_WRITEABLE:
         lwsl_notice("LWS_CALLBACK_SERVER_WRITEABLE: sending %d bytes",pss->len);
         n = libwebsocket_write(wsi, &pss->buf[LWS_SEND_BUFFER_PRE_PADDING], pss->len, LWS_WRITE_BINARY);
         
         if (n < 0)
         {
            lwsl_err("LWS_CALLBACK_SERVER_WRITEABLE: ERROR %d writing to socket, hanging up\n", n);
            return 1;
         }
         if (n < (int)pss->len)
         {
            lwsl_err("LWS_CALLBACK_SERVER_WRITEABLE: Partial write\n");
            return -1;
         }
         break;

      case LWS_CALLBACK_RECEIVE:
         if (len > max_payload)
         {
            lwsl_err("LWS_CALLBACK_RECEIVE: Server received packet bigger than %u, hanging up\n", max_payload);
            return 1;
         }

         lwsl_notice("LWS_CALLBACK_RECEIVE: len=%d", len);
         {
            std::string s1((char*)in, 7);
            bling_pb::header hdr;
            if (hdr.ParseFromString(s1))
               lwsl_notice("LWS_CALLBACK_RECEIVE: header=%s", hdr.ShortDebugString().c_str());
            else
               lwsl_notice("LWS_CALLBACK_RECEIVE error decoding header");
            std::string s2((char*)in+7, len-7);
            std::string s3;
            switch (hdr.msg_id())
            {
               case bling_pb::header::SEND_ALL_STOP:  s3=ms->send_all_stop(s2);  break;
               case bling_pb::header::GET_SLAVE_LIST: s3=ms->get_slave_list(s2); break;
               case bling_pb::header::SET_SLAVE_TLC:  s3=ms->set_slave_tlc(s2);  break;
               case bling_pb::header::START_EFFECT:   s3=ms->start_effect(s2);  break;
               case bling_pb::header::REBOOT_SLAVE:   s3=ms->reboot_slave(s2);  break;
               case bling_pb::header::PING_SLAVE:     s3=ms->ping_slave(s2);  break;
               default:
                  lwsl_notice("LWS_CALLBACK_RECEIVE: Ignoring unknown message.");
            }
            if (s3.length())
            {
               memcpy(&pss->buf[LWS_SEND_BUFFER_PRE_PADDING], s3.c_str(), s3.length());
               pss->len = s3.length();
               libwebsocket_callback_on_writable(context, wsi);
            }
         }
         break;

      case LWS_CALLBACK_CLIENT_ESTABLISHED:
         lwsl_notice("LWS_CALLBACK_CLIENT_ESTABLISHED:");
         pss->index = 0;
         break;

      case LWS_CALLBACK_CLIENT_RECEIVE:
         lwsl_notice("LWS_CALLBACK_CLIENT_RECEIVE:");
         break;

      case LWS_CALLBACK_CLIENT_WRITEABLE:
         lwsl_notice("CALLBACK_CLIENT_WRITEABLE:");
         break;

      case LWS_CALLBACK_CLOSED:
         lwsl_notice("LWS_CALLBACK_CLOSED:");
         break;

      case LWS_CALLBACK_ESTABLISHED:
         lwsl_notice("LWS_CALLBACK_ESTABLISHED:");
         break;

      case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
         lwsl_notice("LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:");
         break;

      case LWS_CALLBACK_CONFIRM_EXTENSION_OKAY:
         lwsl_notice("LWS_CALLBACK_CONFIRM_EXTENSION_OKAY: %s", in);
         if (strcmp((char*)in, "x-webkit-deflate-frame")==0)
         {
            // the node.js stuff doesn't seem to like x-webkit-deflate-frame
            lwsl_notice("LWS_CALLBACK_CONFIRM_EXTENSION_OKAY: denying %s", in);
            return -1;
         }
         break;

      default:
         break;
   }

   return 0;
}


static struct libwebsocket_protocols protocols[] =
{
   { "default", callback_bling_protobuf, sizeof(struct per_session_data__bling_protobuf)},
   { NULL, NULL, 0 }
};


void sighandler(int sig)
{
   force_exit = 1;
}


static struct option options[] = {
   { "help",	no_argument,		NULL, 'h' },
   { "debug",	required_argument,	NULL, 'd' },
   { "port",	required_argument,	NULL, 'p' },
   { "ssl",	no_argument,		NULL, 's' },
   { "interface",  required_argument,	NULL, 'i' },
   { "daemonize", 	no_argument,		NULL, 'D' },
   { NULL, 0, 0, 0 }
};


int main(int argc, char **argv)
{
   GOOGLE_PROTOBUF_VERIFY_VERSION;


   int n = 0;
   int port = 7681;
   int use_ssl = 0;
   struct libwebsocket_context *context;
   int opts = 0;
   char interface_name[128] = "";
   const char *interface = NULL;

   int syslog_options = LOG_PID | LOG_PERROR;

   int client = 0;
   int listen_port;
   struct lws_context_creation_info info;

   char address[256];
   unsigned int oldus = 0;
   struct libwebsocket *wsi;

   int debug_level = 7;
   int daemonize = 0;

   memset(&info, 0, sizeof info);
   
   while (n >= 0)
   {
      n = getopt_long(argc, argv, "i:hsp:d:D", options, NULL);
      if (n < 0)
         continue;
      switch (n)
      {
         case 'D':
            daemonize = 1;
            syslog_options &= ~LOG_PERROR;
            break;
         case 'd':
            debug_level = atoi(optarg);
            break;
         case 's':
            use_ssl = 1; /* 1 = take care about cert verification, 2 = allow anything */
            break;
         case 'p':
            port = atoi(optarg);
            break;
         case 'i':
            strncpy(interface_name, optarg, sizeof interface_name);
            interface_name[(sizeof interface_name) - 1] = '\0';
            interface = interface_name;
            break;
         case '?':
         case 'h':
            fprintf(stderr, "Usage: master_ws "
                    "[--ssl] "
                    "[--port=<p>] "
                    "[-d <log bitfield>]\n");
            exit(1);
      }
   }



   /*
    * normally lock path would be /var/lock/lwsts or similar, to
    * simplify getting started without having to take care about
    * permissions or running as root, set to /tmp/.lwsts-lock
    */
   if (!client && daemonize && lws_daemonize("/tmp/.lwstecho-lock"))
   {
      fprintf(stderr, "Failed to daemonize\n");
      return 1;
   }

   /* we will only try to log things according to our debug_level */
   setlogmask(LOG_UPTO (LOG_DEBUG));
   openlog("lwsts", syslog_options, LOG_DAEMON);

   /* tell the library what debug level to emit and to send it to syslog */
   lws_set_log_level(debug_level, lwsl_emit_syslog);
   lwsl_notice("master_wc - bling master controller with websocket interface");
   if (client)
   {
      lwsl_notice("Running in client mode\n");
      listen_port = CONTEXT_PORT_NO_LISTEN;
      if (use_ssl)
         use_ssl = 2;
   }
   else
   {
      lwsl_notice("Running in server mode\n");
      listen_port = port;
   }

   info.port = listen_port;
   info.iface = interface;
   info.protocols = protocols;
   info.extensions = libwebsocket_get_internal_extensions();
   if (use_ssl && !client)
   {
      info.ssl_cert_filepath = LOCAL_RESOURCE_PATH"/libwebsockets-test-server.pem";
      info.ssl_private_key_filepath = LOCAL_RESOURCE_PATH"/libwebsockets-test-server.key.pem";
   }
   info.gid = -1;
   info.uid = -1;
   info.options = opts;

   ms = new Master_Server();
   ms->debug = debug_level >> 3;

   context = libwebsocket_create_context(&info);

   if (context == NULL)
   {
      lwsl_err("libwebsocket init failed\n");
      return -1;
   }

   signal(SIGINT, sighandler);

   int rc = 0;
   int timeout_ms = 10;
   long t_hb=-1000, t_ping=-1000;
   while (rc >= 0 && !force_exit)
   {
      rc = libwebsocket_service(context, timeout_ms);

      long t=ms->runtime.sec();

      // Send out heartbeat ever second
      if (t != t_hb)
      {
         t_hb=t;
         ms->heartbeat();
      }

      // Ping slave every 5 seconds
      if (t - t_ping >= 5)
      {
         t_ping = t;
         ms->scan();
      }
   }
   
   libwebsocket_context_destroy(context);
   lwsl_notice("master_wc exited cleanly\n");
   closelog();
   delete ms;
   return 0;
}