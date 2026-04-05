/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/test.h"
#include "ns3/evidence_descriptor.h"
#include "ns3/evidence_ring_buffer.h"
#include "ns3/forensic_trigger.h"
#include "ns3/edge_evidence_scheduler.h"

using namespace ns3;

class EvidenceRingBufferFreezeTestCase : public TestCase
{
public:
  EvidenceRingBufferFreezeTestCase ();

private:
  virtual void DoRun (void);
};

EvidenceRingBufferFreezeTestCase::EvidenceRingBufferFreezeTestCase ()
  : TestCase ("freeze exports only evidence inside the requested window")
{
}

void
EvidenceRingBufferFreezeTestCase::DoRun (void)
{
  EvidenceRingBuffer buffer ("veh-1", "vehicle", 10);
  buffer.Push (EvidenceDescriptor ("evt-0", "a", "veh-1", "vehicle", "message", 100, 40, 300, 0.9, 0.5, 0.8, 0.2, 0.1, true));
  buffer.Push (EvidenceDescriptor ("evt-0", "b", "veh-1", "vehicle", "message", 800, 40, 300, 0.9, 0.5, 0.8, 0.2, 0.1, true));
  buffer.Push (EvidenceDescriptor ("evt-0", "c", "veh-1", "vehicle", "message", 1700, 40, 300, 0.9, 0.5, 0.8, 0.2, 0.1, true));

  buffer.FreezeWindow ("evt-1", 1000, 500, 500);
  std::vector<EvidenceDescriptor> frozen = buffer.ExportFrozen ("evt-1");

  NS_TEST_ASSERT_MSG_EQ (frozen.size (), 1u, "Unexpected number of frozen descriptors");
  NS_TEST_ASSERT_MSG_EQ (frozen.front ().GetEvidenceId (), std::string ("b"), "Expected only the middle descriptor");
}

class EdgeEvidenceSchedulerOrderingTestCase : public TestCase
{
public:
  EdgeEvidenceSchedulerOrderingTestCase ();

private:
  virtual void DoRun (void);
};

EdgeEvidenceSchedulerOrderingTestCase::EdgeEvidenceSchedulerOrderingTestCase ()
  : TestCase ("proposed scheduler prefers higher reconstruction value")
{
}

void
EdgeEvidenceSchedulerOrderingTestCase::DoRun (void)
{
  EdgeEvidenceScheduler scheduler;
  scheduler.SetMode ("proposed");
  scheduler.SetUploadBudgetBytes (1000);

  std::vector<EvidenceDescriptor> descriptors;
  descriptors.push_back (EvidenceDescriptor ("evt-1", "low", "veh-1", "vehicle", "message", 100, 100, 400, 0.2, 0.2, 0.8, 0.1, 0.1, true));
  descriptors.push_back (EvidenceDescriptor ("evt-1", "high", "veh-2", "vehicle", "message", 100, 100, 400, 0.9, 0.9, 0.8, 0.8, 0.1, true));

  std::vector<EvidenceDescriptor> selected = scheduler.Select (descriptors, 200);

  NS_TEST_ASSERT_MSG_EQ (selected.size (), 2u, "Expected both descriptors to fit in budget");
  NS_TEST_ASSERT_MSG_EQ (selected.front ().GetEvidenceId (), std::string ("high"), "High-score descriptor should be ranked first");
}

class ForensicsTestSuite : public TestSuite
{
public:
  ForensicsTestSuite ();
};

ForensicsTestSuite::ForensicsTestSuite ()
  : TestSuite ("forensics", UNIT)
{
  AddTestCase (new EvidenceRingBufferFreezeTestCase, TestCase::QUICK);
  AddTestCase (new EdgeEvidenceSchedulerOrderingTestCase, TestCase::QUICK);
}

static ForensicsTestSuite g_forensicsTestSuite;
