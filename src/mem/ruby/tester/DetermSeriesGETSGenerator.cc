
/*
    Copyright (C) 1999-2008 by Mark D. Hill and David A. Wood for the
    Wisconsin Multifacet Project.  Contact: gems@cs.wisc.edu
    http://www.cs.wisc.edu/gems/

    --------------------------------------------------------------------

    This file is part of the Ruby Multiprocessor Memory System Simulator, 
    a component of the Multifacet GEMS (General Execution-driven 
    Multiprocessor Simulator) software toolset originally developed at 
    the University of Wisconsin-Madison.

    Ruby was originally developed primarily by Milo Martin and Daniel
    Sorin with contributions from Ross Dickson, Carl Mauer, and Manoj
    Plakal.

    Substantial further development of Multifacet GEMS at the
    University of Wisconsin was performed by Alaa Alameldeen, Brad
    Beckmann, Jayaram Bobba, Ross Dickson, Dan Gibson, Pacia Harper,
    Derek Hower, Milo Martin, Michael Marty, Carl Mauer, Michelle Moravan,
    Kevin Moore, Andrew Phelps, Manoj Plakal, Daniel Sorin, Haris Volos, 
    Min Xu, and Luke Yen.
    --------------------------------------------------------------------

    If your use of this software contributes to a published paper, we
    request that you (1) cite our summary paper that appears on our
    website (http://www.cs.wisc.edu/gems/) and (2) e-mail a citation
    for your published paper to gems@cs.wisc.edu.

    If you redistribute derivatives of this software, we request that
    you notify us and either (1) ask people to register with us at our
    website (http://www.cs.wisc.edu/gems/) or (2) collect registration
    information and periodically send it to us.

    --------------------------------------------------------------------

    Multifacet GEMS is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    Multifacet GEMS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with the Multifacet GEMS; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307, USA

    The GNU General Public License is contained in the file LICENSE.

### END HEADER ###
*/

/*
 * $Id$
 *
 */

#include "mem/ruby/tester/DetermSeriesGETSGenerator.hh"
#include "mem/protocol/DetermSeriesGETSGeneratorStatus.hh"
#include "mem/ruby/tester/DeterministicDriver.hh"

DetermSeriesGETSGenerator::DetermSeriesGETSGenerator(NodeID node, DeterministicDriver& driver) :
  m_driver(driver)
{
  m_status = DetermSeriesGETSGeneratorStatus_Thinking;
  m_last_transition = 0;
  m_node = node;
  m_address = Address(9999);  // initialize to null value
  m_counter = 0;
  

  // don't know exactly when this node needs to request so just guess randomly
  m_driver.eventQueue->scheduleEvent(this, 1+(random() % 200));
}

DetermSeriesGETSGenerator::~DetermSeriesGETSGenerator()
{
}

void DetermSeriesGETSGenerator::wakeup()
{
  DEBUG_EXPR(TESTER_COMP, MedPrio, m_node);
  DEBUG_EXPR(TESTER_COMP, MedPrio, m_status);

  // determine if this node is next for the SeriesGETS round robin request
  if (m_status == DetermSeriesGETSGeneratorStatus_Thinking) {
    if (m_driver.isLoadReady(m_node)) {
      pickAddress();
      m_status = DetermSeriesGETSGeneratorStatus_Load_Pending;  // Load Pending
      m_last_transition = m_driver.eventQueue->getTime();
      initiateLoad();  // SeriesGETS
    } else { // I'll check again later
      m_driver.eventQueue->scheduleEvent(this, thinkTime());
    }
  } else {
    WARN_EXPR(m_status);
    ERROR_MSG("Invalid status");
  }

}

void DetermSeriesGETSGenerator::performCallback(NodeID proc, Address address)
{
  assert(proc == m_node);
  assert(address == m_address);  

  DEBUG_EXPR(TESTER_COMP, LowPrio, proc);
  DEBUG_EXPR(TESTER_COMP, LowPrio, m_status);
  DEBUG_EXPR(TESTER_COMP, LowPrio, address);

  if (m_status == DetermSeriesGETSGeneratorStatus_Load_Pending) { 
    m_driver.recordLoadLatency(m_driver.eventQueue->getTime() - m_last_transition);
    //data.writeByte(m_node);
    m_driver.loadCompleted(m_node, address);  // advance the load queue

    m_counter++;
    // do we still have more requests to complete before the next proc starts?
    if (m_counter < m_driver.m_tester_length*m_driver.m_numCompletionsPerNode) {
      m_status = DetermSeriesGETSGeneratorStatus_Thinking;
      m_last_transition = m_driver.eventQueue->getTime();
      m_driver.eventQueue->scheduleEvent(this, waitTime());
    } else {
      m_driver.reportDone();
      m_status = DetermSeriesGETSGeneratorStatus_Done;
      m_last_transition = m_driver.eventQueue->getTime();
    } 

  } else {
    WARN_EXPR(m_status);
    ERROR_MSG("Invalid status");
  }
}

int DetermSeriesGETSGenerator::thinkTime() const
{
  return m_driver.m_think_time;
}

int DetermSeriesGETSGenerator::waitTime() const
{
  return m_driver.m_wait_time;
}

void DetermSeriesGETSGenerator::pickAddress()
{
  assert(m_status == DetermSeriesGETSGeneratorStatus_Thinking);

  m_address = m_driver.getNextLoadAddr(m_node);
}

void DetermSeriesGETSGenerator::initiateLoad()
{
  DEBUG_MSG(TESTER_COMP, MedPrio, "initiating Load");
  //sequencer()->makeRequest(CacheMsg(m_address, m_address, CacheRequestType_IFETCH, Address(3), AccessModeType_UserMode, 1, PrefetchBit_No, Address(0), 0 /* only 1 SMT thread */)); 

  uint8_t *read_data = new uint8_t[64];

  char name [] = "Sequencer_";
  char port_name [13];
  sprintf(port_name, "%s%d", name, m_node);

  int64_t request_id = libruby_issue_request(libruby_get_port_by_name(port_name), RubyRequest(m_address.getAddress(), read_data, 64, 0, RubyRequestType_LD, RubyAccessMode_Supervisor));

  //delete [] read_data;

  ASSERT(m_driver.requests.find(request_id) == m_driver.requests.end()); 
  m_driver.requests.insert(make_pair(request_id, make_pair(m_node, m_address)));
}

void DetermSeriesGETSGenerator::print(ostream& out) const
{
}

