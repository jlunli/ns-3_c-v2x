/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "evidence_ring_buffer.h"

namespace ns3 {

EvidenceRingBuffer::EvidenceRingBuffer ()
  : m_capacity (0)
{
}

EvidenceRingBuffer::EvidenceRingBuffer (const std::string& nodeId, const std::string& nodeRole, uint32_t capacity)
  : m_nodeId (nodeId),
    m_nodeRole (nodeRole),
    m_capacity (capacity)
{
}

void
EvidenceRingBuffer::Push (const EvidenceDescriptor& descriptor)
{
  if (m_capacity == 0)
    {
      return;
    }

  while (m_buffer.size () >= m_capacity)
    {
      m_buffer.pop_front ();
    }
  m_buffer.push_back (descriptor);
}

void
EvidenceRingBuffer::FreezeWindow (const std::string& eventId, uint64_t t0Ms, uint64_t preMs, uint64_t postMs)
{
  const uint64_t startMs = (t0Ms > preMs) ? (t0Ms - preMs) : 0;
  const uint64_t endMs = t0Ms + postMs;
  std::vector<EvidenceDescriptor> frozen;
  for (std::deque<EvidenceDescriptor>::const_iterator it = m_buffer.begin (); it != m_buffer.end (); ++it)
    {
      if (it->GetTimestampMs () >= startMs && it->GetTimestampMs () <= endMs)
        {
          frozen.push_back (*it);
        }
    }
  m_frozen[eventId] = frozen;
}

std::vector<EvidenceDescriptor>
EvidenceRingBuffer::ExportFrozen (const std::string& eventId) const
{
  std::map<std::string, std::vector<EvidenceDescriptor> >::const_iterator it = m_frozen.find (eventId);
  if (it == m_frozen.end ())
    {
      return std::vector<EvidenceDescriptor> ();
    }
  return it->second;
}

uint64_t
EvidenceRingBuffer::GetFrozenBytes (const std::string& eventId) const
{
  uint64_t totalBytes = 0;
  std::vector<EvidenceDescriptor> frozen = ExportFrozen (eventId);
  for (std::vector<EvidenceDescriptor>::const_iterator it = frozen.begin (); it != frozen.end (); ++it)
    {
      totalBytes += it->GetSizeBytes ();
    }
  return totalBytes;
}

} // namespace ns3
