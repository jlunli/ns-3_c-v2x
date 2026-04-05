/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "forensic_trigger.h"

#include <sstream>

namespace ns3 {

ForensicTrigger::ForensicTrigger ()
  : m_attackStartMs (0),
    m_burstCount (0),
    m_eventCounter (0)
{
}

void
ForensicTrigger::ConfigureAttack (const std::string& attackerId, uint64_t attackStartMs, uint32_t burstCount)
{
  m_attackerId = attackerId;
  m_attackStartMs = attackStartMs;
  m_burstCount = burstCount;
}

bool
ForensicTrigger::OnTx (const std::string& nodeId,
                       const std::string& role,
                       const std::string& messageType,
                       uint64_t timestampMs,
                       bool isForgedEmergency)
{
  if (!isForgedEmergency || nodeId != m_attackerId || messageType != "forged_emergency_brake")
    {
      return false;
    }

  if (m_activeEventId.empty ())
    {
      std::ostringstream builder;
      builder << "evt-" << m_eventCounter++;
      m_activeEventId = builder.str ();
    }

  RecordTrigger (nodeId, role, "attacker_tx", timestampMs);
  return true;
}

bool
ForensicTrigger::OnRx (const std::string& nodeId,
                       const std::string& role,
                       const std::string& claimedSenderId,
                       uint64_t timestampMs,
                       bool suspiciousEmergency)
{
  if (!suspiciousEmergency)
    {
      return false;
    }

  if (m_activeEventId.empty ())
    {
      std::ostringstream builder;
      builder << "evt-" << m_eventCounter++;
      m_activeEventId = builder.str ();
    }

  std::string reason = (claimedSenderId == m_attackerId) ? "suspicious_rx" : "conflicting_rx";
  RecordTrigger (nodeId, role, reason, timestampMs);
  return true;
}

bool
ForensicTrigger::ShouldFreeze (const std::string& nodeId) const
{
  std::map<std::string, bool>::const_iterator it = m_frozenNodes.find (nodeId);
  return it != m_frozenNodes.end () && it->second;
}

std::string
ForensicTrigger::GetActiveEventId () const
{
  return m_activeEventId;
}

std::vector<TriggerLogEntry>
ForensicTrigger::GetTriggerLog () const
{
  return m_log;
}

void
ForensicTrigger::RecordTrigger (const std::string& nodeId,
                                const std::string& role,
                                const std::string& reason,
                                uint64_t timestampMs)
{
  if (ShouldFreeze (nodeId))
    {
      return;
    }

  m_frozenNodes[nodeId] = true;
  TriggerLogEntry entry;
  entry.eventId = m_activeEventId;
  entry.nodeId = nodeId;
  entry.role = role;
  entry.reason = reason;
  entry.triggerTimeMs = timestampMs;
  m_log.push_back (entry);
}

} // namespace ns3
