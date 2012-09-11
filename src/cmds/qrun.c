#include "license_pbs.h" /* See here for the software license */
/*
 * qrun
 *  The qrun command forces a batch job to run.
 *
 * Synopsis:
 *  qrun [-H host] job_identifier ...
 *
 * Arguments:
 *  host
 *      The host to run the job at.
 *  job_identifier ...
 *      A list of job_identifiers.  A job_identifier has the following form:
 *          sequence_number[.server_name][@server]
 *
 * Written by:
 *  Bruce Kelly
 *  National Energy Research Supercomputer Center
 *  Livermore, CA
 *  May, 1993
 */

#include "cmds.h"
#include "net_cache.h"
#include <pbs_config.h>   /* the master config generated by configure */

int exitstatus = 0; /* Exit Status */
static void execute(char *, char *, char *, int);



int main(

  int    argc,  /* I */
  char **argv)  /* I */

  {
  /*
   *  This routine sends a Run Job request to the batch server.  If the
   * batch request is accepted, the server will have started the execution
   * of the job.
   */

  char job[PBS_MAXCLTJOBID];      /* Job Id */
  char server[MAXSERVERNAME];   /* Server name */
  char *location = NULL;          /* Where to run the job */

  static char opts[] = "aH:";     /* See man getopt */
  static char *usage = "Usage: qrun [-a] [-H host] job_id ...\n";
  int s;
  int errflg = 0;
  int runAsync = FALSE;

  initialize_network_info();

  /* Command line options */

  while ((s = getopt(argc, argv, opts)) != EOF)
    {
    switch (s)
      {

      case 'a': /* Async job start */

        runAsync = TRUE;

        break;

      case 'H':

        if (strlen(optarg) == 0)
          {
          fprintf(stderr, "qrun: illegal -H value\n");
          errflg++;
          break;
          }

        location = optarg;

        break;

      case '?':

      default:

        errflg++;

        break;
      }  /* END switch(s) */
    }    /* END while (getopt() != -1) */

  if (errflg || (optind >= argc))
    {
    fprintf(stderr,"%s", usage);

    exit(1);
    }

  for (;optind < argc;optind++)
    {
    if (get_server(argv[optind], job, sizeof(job), server, sizeof(server)))
      {
      fprintf(stderr, "qrun: illegally formed job identifier: %s\n",
              argv[optind]);

      exitstatus = 1;

      continue;
      }

    execute(job, server, location, runAsync);
    }  /* END for (;optind) */

  exit(exitstatus);

  /*NOTREACHED*/

  return(0);
  }  /* END main() */





/*
 * void execute(char *job,char *server,char *location)
 *
 * job      The fully qualified job id.
 * server   The name of the server that manages the job.
 * location The name of the server to run the job.
 *
 * Returns:
 *  None
 *
 * File Variables:
 *  exitstatus  Set to two if an error occurs.
 */

static void execute(

  char *job,      /* I */
  char *server,   /* I */
  char *location, /* I */
  int  async)     /* I */

  {
  int ct;         /* Connection to the server */
  int err;        /* Error return from pbs_run */
  int located = FALSE;
  char rmt_server[MAXSERVERNAME];
  int  local_errno = 0;

cnt:

  if ((ct = cnt2server(server)) > 0)
    {
    if (async == TRUE)
      {
      err = pbs_asyrunjob_err(ct, job, location, NULL, &local_errno);  /* see lib/Libifl/pbsD_runjob.c */
      }
    else
      {
      err = pbs_runjob_err(ct, job, location, NULL, &local_errno);  /* see lib/Libifl/pbsD_runjob.c */
      }

    if (err && (local_errno == PBSE_UNKNODE))
      {
      fprintf(stderr, "qrun: Unknown node in hostlist '%.16s...' for job %s\n",
              location,
              job);

      exitstatus = 2;
      }
    else if (err && (local_errno != PBSE_UNKJOBID))
      {
      prt_job_err("qrun", ct, job);

      exitstatus = 2;
      }
    else if (err && (local_errno == PBSE_UNKJOBID) && !located)
      {
      located = TRUE;

      if (locate_job(job, server, rmt_server))
        {
        pbs_disconnect(ct);

        strcpy(server, rmt_server);

        goto cnt;
        }

      prt_job_err("qrun", ct, job);

      exitstatus = 2;
      }

    pbs_disconnect(ct);
    }
  else
    {
    fprintf(stderr, "qrun: could not connect to server %s (%d) %s\n",
            server,
            ct * -1,
            pbs_strerror(ct * -1));

    exitstatus = 2;
    }

  return;
  }  /* END execute() */

/* END qrun.c */

