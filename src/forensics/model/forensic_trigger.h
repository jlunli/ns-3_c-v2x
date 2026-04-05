/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef FORENSIC_TRIGGER_H
#define FORENSIC_TRIGGER_H

#include <map>
#include <string>
#include <vector>

namespace ns3 {

struct TriggerLogEntry
{
  std::string eventId;
  std::string nodeId;
  std::string role;
  std::string reason;
  uint64_t triggerTimeMs;
};

class ForensicTrigger
{
public:
  ForensicTrigger ();

  void ConfigureAttack (const std::string& attackerId, uint64_t attackStartMs, uint32_t burstCount);
  bool OnTx (const std::string& nodeId,
             const std::string& role,
             const std::string& messageType,
             uint64_t timestampMs,
             bool isForgedEmergency);
  bool OnRx (const std::string& nodeId,
             const std::string& role,
             const std::string& claimedSenderId,
             uint64_t timestampMs,
             bool suspiciousEmergency);
  bool ShouldFreeze (const std::string& nodeId) const;
  std::string GetActiveEventId () const;
  std::vector<TriggerLogEntry> GetTriggerLog () const;

private:
  void RecordTrigger (const std::string& nodeId,
                      const std::string& role,
                      const std::string& reason,
                      uint64_t timestampMs);

  std::string m_attackerId;
  std::string m_activeEventId;
  uint64_t m_attackStartMs;
  uint32_t m_burstCount;
  uint32_t m_eventCounter;
  std::map<std::string, bool> m_frozenNodes;
  std::vector<TriggerLogEntry> m_log;
};

} // namespace ns3

#endif /* FORENSIC_TRIGGER_H */
