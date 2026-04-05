/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "ns3/core-module.h"
#include "ns3/mobility-module.h"
#include "ns3/network-module.h"
#include "ns3/evidence_descriptor.h"
#include "ns3/evidence_ring_buffer.h"
#include "ns3/forensic_trigger.h"
#include "ns3/edge_evidence_scheduler.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <vector>

using namespace ns3;

namespace {

struct NodeContext
{
  std::string id;
  std::string role;
  EvidenceRingBuffer ring;
};

struct UploadEntry
{
  EvidenceDescriptor descriptor;
  uint64_t arrivalMs;
};

double Clamp01 (double value) { return std::max (0.0, std::min (1.0, value)); }

uint32_t AttackBurstCount (const std::string& attackRate)
{
  return attackRate == "low" ? 1 : (attackRate == "high" ? 5 : 3);
}

double DeadlineUrgency (uint64_t timestampMs, uint64_t deadlineMs)
{
  if (deadlineMs <= timestampMs)
    {
      return 1.0;
    }
  return Clamp01 (1.0 - static_cast<double> (deadlineMs - timestampMs) / 1200.0);
}

std::string Label (const std::string& prefix, uint32_t index)
{
  std::ostringstream stream;
  stream << prefix << "-" << index;
  return stream.str ();
}

EvidenceDescriptor MakeDescriptor (const std::string& eventId,
                                   const std::string& evidenceId,
                                   const std::string& sourceId,
                                   const std::string& role,
                                   const std::string& type,
                                   uint64_t timestampMs,
                                   uint32_t sizeBytes,
                                   uint64_t deadlineMs,
                                   double relevance,
                                   double credibility,
                                   double complementarity,
                                   double cost,
                                   bool isKeyTarget)
{
  return EvidenceDescriptor (eventId,
                             evidenceId,
                             sourceId,
                             role,
                             type,
                             timestampMs,
                             sizeBytes,
                             deadlineMs,
                             relevance,
                             DeadlineUrgency (timestampMs, deadlineMs),
                             credibility,
                             complementarity,
                             cost,
                             isKeyTarget);
}

void WriteMetricsCsv (const std::string& outputDir,
                      uint32_t vehicles,
                      uint32_t rsu,
                      const std::string& attackRate,
                      const std::string& scheduler,
                      uint32_t seed,
                      double coverage,
                      double deadline,
                      double timeline,
                      double attribution,
                      double overhead,
                      uint32_t keyTotal,
                      uint32_t keyUploaded,
                      uint32_t keyOnTime,
                      uint64_t uploadBytes,
                      uint64_t baselineServiceBytes)
{
  std::ofstream stream ((outputDir + "/run_metrics.csv").c_str (), std::ios::out | std::ios::trunc);
  stream << "vehicles,rsu,attack_rate,scheduler,seed,key_evidence_coverage_rate,key_evidence_deadline_satisfaction_ratio,timeline_reconstruction_success_rate,attribution_accuracy,normal_service_overhead_ratio,key_evidence_total,key_evidence_uploaded,key_evidence_on_time,upload_bytes,baseline_service_bytes\n";
  stream << vehicles << "," << rsu << "," << attackRate << "," << scheduler << "," << seed << ","
         << std::fixed << std::setprecision (4)
         << coverage << "," << deadline << "," << timeline << "," << attribution << "," << overhead << ","
         << keyTotal << "," << keyUploaded << "," << keyOnTime << "," << uploadBytes << "," << baselineServiceBytes << "\n";
}

} // namespace

int main (int argc, char* argv[])
{
  uint32_t vehicles = 20;
  uint32_t rsu = 2;
  uint32_t seed = 1;
  std::string attackRate = "medium";
  std::string schedulerMode = "proposed";
  std::string outputDir = ".";
  uint64_t attackStartMs = 2000;
  uint64_t uploadBudgetBytes = 2200;

  CommandLine cmd;
  cmd.AddValue ("vehicles", "Number of vehicles", vehicles);
  cmd.AddValue ("rsu", "Number of RSUs", rsu);
  cmd.AddValue ("attackRate", "Attack intensity", attackRate);
  cmd.AddValue ("scheduler", "Scheduler mode", schedulerMode);
  cmd.AddValue ("seed", "Run seed", seed);
  cmd.AddValue ("outputDir", "Output directory", outputDir);
  cmd.AddValue ("uploadBudgetBytes", "Upload budget in bytes", uploadBudgetBytes);
  cmd.Parse (argc, argv);

  RngSeedManager::SetSeed (12345);
  RngSeedManager::SetRun (seed);

  NodeContainer vehicleNodes;
  vehicleNodes.Create (vehicles);
  NodeContainer rsuNodes;
  rsuNodes.Create (rsu);
  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (vehicleNodes);
  mobility.Install (rsuNodes);

  std::vector<NodeContext> contexts;
  contexts.reserve (vehicles + rsu);
  for (uint32_t i = 0; i < vehicles; ++i)
    {
      NodeContext ctx;
      ctx.id = Label ("veh", i);
      ctx.role = "vehicle";
      ctx.ring = EvidenceRingBuffer (ctx.id, ctx.role, 64);
      contexts.push_back (ctx);
    }
  for (uint32_t i = 0; i < rsu; ++i)
    {
      NodeContext ctx;
      ctx.id = Label ("rsu", i);
      ctx.role = "rsu";
      ctx.ring = EvidenceRingBuffer (ctx.id, ctx.role, 64);
      contexts.push_back (ctx);
    }

  std::map<std::string, NodeContext*> nodeById;
  for (std::vector<NodeContext>::iterator it = contexts.begin (); it != contexts.end (); ++it)
    {
      nodeById[it->id] = &(*it);
    }

  const std::string attackerId = Label ("veh", vehicles / 3);
  const uint32_t burstCount = AttackBurstCount (attackRate);
  const uint32_t affectedVehicles = std::min<uint32_t> (vehicles > 1 ? vehicles - 1 : 0, 3 + burstCount + rsu);
  uint32_t evidenceCounter = 0;

  ForensicTrigger trigger;
  trigger.ConfigureAttack (attackerId, attackStartMs, burstCount);
  trigger.OnTx (attackerId, "vehicle", "forged_emergency_brake", attackStartMs, true);
  const std::string eventId = trigger.GetActiveEventId ();

  std::vector<EvidenceDescriptor> oracleKeyEvidence;
  for (std::vector<NodeContext>::iterator it = contexts.begin (); it != contexts.end (); ++it)
    {
      for (uint32_t sample = 0; sample < 4 + burstCount; ++sample)
        {
          const uint64_t t = attackStartMs - 300 + sample * 60;
          it->ring.Push (MakeDescriptor (eventId, Label ("distractor", evidenceCounter++), it->id, it->role, "routine_summary", t, 72, attackStartMs + 260 + sample * 10, 0.15, 0.70, 0.10, 0.08, false));
        }
    }

  EvidenceDescriptor forgedTx = MakeDescriptor (eventId, Label ("tx", evidenceCounter++), attackerId, "vehicle", "forged_tx", attackStartMs, 220, attackStartMs + 900, 0.98, 0.92, 0.95, 0.35, true);
  nodeById[attackerId]->ring.Push (forgedTx);
  oracleKeyEvidence.push_back (forgedTx);

  for (uint32_t i = 0; i < affectedVehicles; ++i)
    {
      const std::string receiverId = Label ("veh", (vehicles / 3 + i + 1) % vehicles);
      if (receiverId == attackerId)
        {
          continue;
        }
      const uint64_t rxTime = attackStartMs + 30 + i * 18;
      const uint64_t brakeTime = rxTime + 90 + (i % 3) * 15;
      trigger.OnRx (receiverId, "vehicle", attackerId, rxTime, true);

      EvidenceDescriptor rxEvidence = MakeDescriptor (eventId, Label ("rx", evidenceCounter++), receiverId, "vehicle", "receiver_first_rx", rxTime, 190, attackStartMs + 840, 0.92, 0.75, 0.83, 0.26, true);
      EvidenceDescriptor brakeEvidence = MakeDescriptor (eventId, Label ("brake", evidenceCounter++), receiverId, "vehicle", "brake_state_change", brakeTime, 210, attackStartMs + 980, 0.95, 0.92, 0.88, 0.32, true);
      nodeById[receiverId]->ring.Push (rxEvidence);
      nodeById[receiverId]->ring.Push (brakeEvidence);
      oracleKeyEvidence.push_back (rxEvidence);
      oracleKeyEvidence.push_back (brakeEvidence);
    }

  for (uint32_t i = 0; i < rsu; ++i)
    {
      const std::string rsuId = Label ("rsu", i);
      const uint64_t obsTime = attackStartMs + 22 + i * 11;
      trigger.OnRx (rsuId, "rsu", attackerId, obsTime, true);
      EvidenceDescriptor rsuEvidence = MakeDescriptor (eventId, Label ("rsu-evidence", evidenceCounter++), rsuId, "rsu", "rsu_first_observation", obsTime, 160, attackStartMs + 920, 0.86, 0.84, 0.90, 0.22, true);
      nodeById[rsuId]->ring.Push (rsuEvidence);
      oracleKeyEvidence.push_back (rsuEvidence);
    }

  for (std::vector<NodeContext>::iterator it = contexts.begin (); it != contexts.end (); ++it)
    {
      if (trigger.ShouldFreeze (it->id))
        {
          it->ring.FreezeWindow (eventId, attackStartMs, 500, 1000);
        }
    }

  std::vector<EvidenceDescriptor> exported;
  for (std::vector<NodeContext>::const_iterator it = contexts.begin (); it != contexts.end (); ++it)
    {
      std::vector<EvidenceDescriptor> frozen = it->ring.ExportFrozen (eventId);
      exported.insert (exported.end (), frozen.begin (), frozen.end ());
    }

  EdgeEvidenceScheduler scheduler;
  scheduler.SetMode (schedulerMode);
  scheduler.SetUploadBudgetBytes (uploadBudgetBytes + rsu * 120);
  const uint64_t baselineServiceBytes = static_cast<uint64_t> (vehicles * 10 * 190 + rsu * 6 * 140);
  std::vector<EvidenceDescriptor> selected = scheduler.Select (exported, baselineServiceBytes);

  std::vector<UploadEntry> uploads;
  uint64_t arrivalMs = attackStartMs + 150;
  for (std::vector<EvidenceDescriptor>::const_iterator it = selected.begin (); it != selected.end (); ++it)
    {
      arrivalMs += 20 + it->GetSizeBytes () / 6;
      UploadEntry upload;
      upload.descriptor = *it;
      upload.arrivalMs = arrivalMs;
      uploads.push_back (upload);
    }

  std::set<std::string> uploadedKeys;
  std::set<std::string> onTimeKeys;
  bool hasTx = false;
  bool hasRx = false;
  bool hasBrake = false;
  bool hasRsu = false;
  uint64_t uploadBytes = 0;
  for (std::vector<UploadEntry>::const_iterator it = uploads.begin (); it != uploads.end (); ++it)
    {
      uploadBytes += it->descriptor.GetSizeBytes ();
      if (it->descriptor.IsKeyTarget ())
        {
          uploadedKeys.insert (it->descriptor.GetEvidenceId ());
          if (it->descriptor.DeadlineSatisfied (it->arrivalMs))
            {
              onTimeKeys.insert (it->descriptor.GetEvidenceId ());
            }
        }
      hasTx = hasTx || it->descriptor.GetType () == "forged_tx";
      hasRx = hasRx || it->descriptor.GetType () == "receiver_first_rx";
      hasBrake = hasBrake || it->descriptor.GetType () == "brake_state_change";
      hasRsu = hasRsu || it->descriptor.GetType () == "rsu_first_observation";
    }

  const uint32_t keyTotal = static_cast<uint32_t> (oracleKeyEvidence.size ());
  const uint32_t keyUploaded = static_cast<uint32_t> (uploadedKeys.size ());
  const uint32_t keyOnTime = static_cast<uint32_t> (onTimeKeys.size ());
  const double coverage = keyTotal > 0 ? static_cast<double> (keyUploaded) / keyTotal : 0.0;
  const double deadline = keyTotal > 0 ? static_cast<double> (keyOnTime) / keyTotal : 0.0;
  const double attribution = (hasTx || (hasRx && hasRsu)) ? 1.0 : 0.0;
  const double timelineEdges = (hasTx ? 1.0 : 0.0) + (hasRx ? 1.0 : 0.0) + (hasBrake ? 1.0 : 0.0) + (hasRsu ? 1.0 : 0.0);
  const double timeline = (attribution >= 1.0 && (timelineEdges / 4.0) >= 0.8) ? 1.0 : 0.0;
  const double overhead = baselineServiceBytes > 0 ? (uploads.size () * 24.0 + static_cast<double> (uploadBytes)) / static_cast<double> (baselineServiceBytes) : 0.0;

  WriteMetricsCsv (outputDir, vehicles, rsu, attackRate, schedulerMode, seed, coverage, deadline, timeline, attribution, overhead, keyTotal, keyUploaded, keyOnTime, uploadBytes, baselineServiceBytes);

  std::cout << "coverage=" << coverage
            << " deadline=" << deadline
            << " timeline=" << timeline
            << " attribution=" << attribution
            << " overhead=" << overhead
            << std::endl;
  return 0;
}
