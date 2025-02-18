#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>

#include "tools.h"
#include "control.h"
#include "version.h"

int main(int argc, char** argv) {

  int c;
  int ctl_sock;
  int show_usage = 0;
  unsigned int msglen = 0;
  dhcp_option* option = NULL;

  if (argc == 1) {
    show_usage = 1;
  }

  char* path = (char*)"/tmp/ddhcpd_ctl";

#define BUFSIZE_MAX 1500
  uint8_t* buffer = calloc(BUFSIZE_MAX, 1);

  if (!buffer) {
    fprintf(stderr, "Failed to allocate message buffer\n");
    exit(1);
  }

  while ((c = getopt(argc, argv, "bC:dhl:o:r:sSt:v:V")) != -1) {
    switch (c) {
    case 'h':
      show_usage = 1;
      break;

    case 'b':
      //show blocks
      msglen = 1;
      buffer[0] = (uint8_t) DDHCPCTL_BLOCK_SHOW;
      break;

    case 'd':
      // show dhcp
      msglen = 1;
      buffer[0] = (uint8_t) DDHCPCTL_DHCP_OPTIONS_SHOW;
      break;

#ifdef DDHCPD_STATISTICS
    case 's':
      msglen = 1;
      buffer[0] = (uint8_t) DDHCPCTL_STATISTICS;
      break;

    case 'S':
      msglen = 1;
      buffer[0] = (uint8_t) DDHCPCTL_STATISTICS_RESET;
      break;
#endif

    case 'o':
      option = parse_option();
      break;

    case 'l':
      msglen = 7;
      buffer[0] = (uint8_t) DDHCPCTL_DHCP_OPTION_SET;
      buffer[1] = (uint8_t) 51;
      buffer[2] = (uint8_t) 4;
      uint32_t leasetime = htonl((uint32_t)strtoul(optarg, NULL, 0));
      memcpy(buffer + 3, (uint8_t*) &leasetime, sizeof(uint32_t));
      break;

    case 'r':
      msglen = 2;
      buffer[0] = (uint8_t) DDHCPCTL_DHCP_OPTION_REMOVE;
      buffer[1] = (uint8_t) atoi(optarg);
      break;

    case 'C':
      path = optarg;
      break;

    case 'v':
      msglen = 2;
      buffer[0] = (uint8_t) DDHCPCTL_LOG_LEVEL_SET;
      buffer[1] = (uint8_t) atoi(optarg);
      break;

    case 'V':
      printf("Revision: %s\n", REVISION);
      return 0;

    default:
      printf("ARGC: %i\n", argc);
      show_usage = 1;
      break;
    }
  }

  // Check if a dhcp option code should be set and if all parameters
  // for that are given.
  if (option) {
    msglen = 3u + option->len;
    buffer[0] = (uint8_t) 3;
    buffer[1] = (uint8_t) option->code;
    buffer[2] = (uint8_t) option->len;
    memcpy(buffer + 3, option->payload, option->len);
    free(option);
  }

  if (show_usage) {
    printf("Usage: ddhcpctl [-h|-V|-b|-d|-o <option>|-C PATH|-l TIMEOUT|-v VERBOSITY]\n");
    printf("\n");
    printf("-h                     This usage information\n");
    printf("-V                     Print build revision\n");
    printf("-b                     Show current block usage.\n");
    printf("-d                     Show the current dhcp options store.\n");
    printf("-l TIMEOUT             Set the dhcp lease time.\n");
    printf("-o CODE:LEN:P1. .. .Pn Set DHCP Option with code,len and #len chars in decimal\n");
    printf("-v LEVEL               Set log level\n");
    printf("-r CODE                Remove DHCP Option");
    printf("-C PATH                Path to control socket\n");
#ifdef DDHCPD_STATISTICS
    printf("-s                    Print statistics\n");
    printf("-S                    Print statistics and reset values\n");
#endif
    exit(0);
  }

  if ((ctl_sock = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) < 0) {
    perror("can't create socket");
    free(buffer);
    return (-1);
  }

  struct sockaddr_un s_un = { 0 };

  s_un.sun_family = AF_UNIX;


  strncpy(s_un.sun_path, path, sizeof(s_un.sun_path));

  if (connect(ctl_sock, (struct sockaddr*)&s_un, sizeof(s_un)) < 0) {
    perror("can't connect to control socket");
    free(buffer);
    close(ctl_sock);
    return -1;
  }

  int ret = fcntl(ctl_sock, F_GETFL, 0);

  if (ret < 0) {
    perror("Can't set stuff");
  }

  ssize_t bw = send(ctl_sock, buffer, msglen, 0);

  if (bw < (ssize_t)msglen) {
    printf("Wrote %i / %u bytes to control socket", (int) bw, msglen);
    perror("send error:");
    return -1;
  }

  ssize_t br;

  while ((br = recv(ctl_sock, (char*) buffer, BUFSIZE_MAX - 1, 0))) {
    buffer[br] = '\0';
    printf("%s", (char*) buffer);
  }

  close(ctl_sock);
  free(buffer);
}
