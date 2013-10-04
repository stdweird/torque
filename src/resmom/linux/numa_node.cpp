/*
*         OpenPBS (Portable Batch System) v2.3 Software License
*
* Copyright (c) 1999-2000 Veridian Information Solutions, Inc.
* All rights reserved.
*
* ---------------------------------------------------------------------------
* For a license to use or redistribute the OpenPBS software under conditions
* other than those described below, or to purchase support for this software,
* please contact Veridian Systems, PBS Products Department ("Licensor") at:
*
*    www.OpenPBS.org  +1 650 967-4675                  sales@OpenPBS.org
*                        877 902-4PBS (US toll-free)
* ---------------------------------------------------------------------------
*
* This license covers use of the OpenPBS v2.3 software (the "Software") at
* your site or location, and, for certain users, redistribution of the
* Software to other sites and locations.  Use and redistribution of
* OpenPBS v2.3 in source and binary forms, with or without modification,
* are permitted provided that all of the following conditions are met.
* After December 31, 2001, only conditions 3-6 must be met:
*
* 1. Commercial and/or non-commercial use of the Software is permitted
*    provided a current software registration is on file at www.OpenPBS.org.
*    If use of this software contributes to a publication, product, or
*    service, proper attribution must be given; see www.OpenPBS.org/credit.html
*
* 2. Redistribution in any form is only permitted for non-commercial,
*    non-profit purposes.  There can be no charge for the Software or any
*    software incorporating the Software.  Further, there can be no
*    expectation of revenue generated as a consequence of redistributing
*    the Software.
*
* 3. Any Redistribution of source code must retain the above copyright notice
*    and the acknowledgment contained in paragraph 6, this list of conditions
*    and the disclaimer contained in paragraph 7.
*
* 4. Any Redistribution in binary form must reproduce the above copyright
*    notice and the acknowledgment contained in paragraph 6, this list of
*    conditions and the disclaimer contained in paragraph 7 in the
*    documentation and/or other materials provided with the distribution.
*
* 5. Redistributions in any form must be accompanied by information on how to
*    obtain complete source code for the OpenPBS software and any
*    modifications and/or additions to the OpenPBS software.  The source code
*    must either be included in the distribution or be available for no more
*    than the cost of distribution plus a nominal fee, and all modifications
*    and additions to the Software must be freely redistributable by any party
*    (including Licensor) without restriction.
*
* 6. All advertising materials mentioning features or use of the Software must
*    display the following acknowledgment:
*
*     "This product includes software developed by NASA Ames Research Center,
*     Lawrence Livermore National Laboratory, and Veridian Information
*     Solutions, Inc.
*     Visit www.OpenPBS.org for OpenPBS software support,
*     products, and information."
*
* 7. DISCLAIMER OF WARRANTY
*
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND. ANY EXPRESS
* OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT
* ARE EXPRESSLY DISCLAIMED.
*
* IN NO EVENT SHALL VERIDIAN CORPORATION, ITS AFFILIATED COMPANIES, OR THE
* U.S. GOVERNMENT OR ANY OF ITS AGENCIES BE LIABLE FOR ANY DIRECT OR INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
* OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
* NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
* EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* This license will be governed by the laws of the Commonwealth of Virginia,
* without reference to its choice of law rules.
*/


#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>

#include "numa_node.hpp"
#include "utils.h"
#include "mom_memory.h"




/*
 * parse_cpu_string
 * parses a string for indices in the format int[-int][,int[-int]...]
 *
 * @pre-cond: str is a valid string pointer
 * @post-cond: internal variables representing cpus are populated
 */
void numa_node::parse_cpu_string(

  std::string &line)

  {
  char       *str = strdup(line.c_str());
  char       *ptr = str;
  int         prev = -1;
  int         curr = -1;
  bool        parsing_range = false;

  this->total_cpus = 0;
  this->available_cpus = 0;

  while (*ptr != '\0')
    {
    prev = strtol(ptr, &ptr, 10);

    if (*ptr == '-')
      {
      ptr++;
      curr = strtol(ptr, &ptr, 10);

      while (prev <= curr)
        {
        this->cpu_indices.push_back(prev);
        this->cpu_avail.push_back(true);
        this->total_cpus++;
        this->available_cpus++;
        prev++;
        }

      if (*ptr == ',')
        ptr++;
      }
    else if ((*ptr == ',') ||
             (*ptr == '\0'))
      {
      this->cpu_indices.push_back(prev);
      this->cpu_avail.push_back(true);
      this->total_cpus++;
      this->available_cpus++;
      ptr++;
      }
    }

  free(str);
  }



/*
 * get_cpuinfo()
 * opens the path to populate internal cpu values
 *
 * @pre-cond: path must be a valid string pointer
 * @post-cond: internal values for cpus are populated
 */
void numa_node::get_cpuinfo(
    
  const char *path)

  {
  if (path == NULL)
    return;

  unsigned int       total_cpus;
  unsigned long      total_memory;
  unsigned long      available_memory;
  unsigned int       available_cpus;

  std::string   line;
  std::ifstream myfile(path);

  if (myfile.is_open())
    {
    getline(myfile, line);

    /* format int[-int][,int[-int]...] */
    parse_cpu_string(line);
    }
  }

void numa_node::get_meminfo(

  const char *path)

  {
  if (path == NULL)
    return;

  proc_mem_t *memnode = get_proc_mem_from_path(path);

  if (memnode != NULL)
    {
    this->total_memory = memnode->mem_total / 1024;
    this->available_memory = this->total_memory;
    }
  }


numa_node::numa_node(

  const numa_node &nn) : total_cpus(nn.total_cpus), total_memory(nn.total_memory),
                         available_memory(nn.available_memory), available_cpus(nn.available_cpus),
                         my_index(nn.my_index), cpu_indices(nn.cpu_indices), cpu_avail(nn.cpu_avail),
                         allocations(nn.allocations)

  {
  }
  
numa_node::numa_node(
    
  const char *node_path,
  int         index) : my_index(index)

  {
  char path[MAXLINE];

  snprintf(path, sizeof(path), "%s/cpulist", node_path);
  get_cpuinfo(path);

  snprintf(path, sizeof(path), "%s/meminfo", node_path);
  get_meminfo(path);
  }


numa_node::numa_node() : total_cpus(0), total_memory (0), available_memory(0), available_cpus(0), my_index(0),
                         cpu_indices(), cpu_avail(), allocations()

  {
  }


bool numa_node::completely_fits(

  int           num_cpus,
  unsigned long memory)

  {
  bool fits = false;

  if ((num_cpus <= this->available_cpus) &&
      (memory <= this->available_memory))
    fits = true;

  return(fits);
  }



void numa_node::reserve(
    
  int            num_cpus,
  unsigned long  memory,
  const char    *jobid,
  allocation    &alloc)

  {
  snprintf(alloc.jobid, sizeof(alloc.jobid), "%s", jobid);
  
  for (unsigned int i = 0; i < this->cpu_indices.size() && alloc.cpus < num_cpus; i++)
    {
    if (this->cpu_avail[i] == true)
      {
      this->cpu_avail[i] = false;
      alloc.cpu_indices.push_back(this->cpu_indices[i]);
      alloc.cpus++;
      this->available_cpus--;
      }
    }

  if (memory <= this->available_memory)
    {
    alloc.memory += memory;
    this->available_memory -= memory;
    }
  else
    {
    alloc.memory += this->available_memory;
    this->available_memory = 0;
    }

  this->allocations.push_back(alloc);
  }
  


void numa_node::remove_job(
    
  const char *jobid)

  {
  for (unsigned int i = 0; i < this->allocations.size(); i++)
    {
    if (!strcmp(jobid, this->allocations[i].jobid))
      {
      allocation a = this->allocations[i];
      this->available_cpus   += a.cpus;
      this->available_memory += a.memory;

      /* find the job indices used by this allocation and release them */
      for (unsigned int k = 0; k < this->cpu_indices.size(); k++)
        {
        for (unsigned int j = 0; j < a.cpu_indices.size(); j++)
          {
          if (this->cpu_indices[k] == a.cpu_indices[j])
            this->cpu_avail[k] = true;
          }
        }

      this->allocations.erase(this->allocations.begin() + i);
      break;
      }
    }
  }



void numa_node::get_job_indices(

  const char       *jobid,
  std::vector<int> &indices,
  bool              cpus)

  {
  for (unsigned int i = 0; i < this->allocations.size(); i++)
    {
    if (!strcmp(jobid, this->allocations[i].jobid))
      {
      if (cpus)
        {
        for (unsigned int j = 0; j < this->allocations[i].cpu_indices.size(); j++)
          indices.push_back(this->allocations[i].cpu_indices[j]);
        }
      else
        indices.push_back(this->my_index);
      }
    }
  }

unsigned int numa_node::get_total_cpus() const

  {
  return(this->total_cpus);
  }

unsigned long numa_node::get_total_memory() const

  {
  return(this->total_memory);
  }

unsigned long numa_node::get_available_memory() const

  {
  return(this->available_memory);
  }

unsigned int numa_node::get_available_cpus() const

  {
  return(this->available_cpus);
  }

unsigned int numa_node::get_my_index() const

  {
  return(this->my_index);
  }

allocation::allocation(

  const allocation &alloc) : memory(alloc.memory), cpus(alloc.cpus), cpu_indices(alloc.cpu_indices)

  {
  strcpy(this->jobid, alloc.jobid);
  }

allocation::allocation() : memory(0), cpus(0), cpu_indices()

  {
  this->jobid[0] = '\0';
  }