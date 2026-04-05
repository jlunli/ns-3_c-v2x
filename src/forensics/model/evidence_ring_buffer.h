/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef EVIDENCE_RING_BUFFER_H
#define EVIDENCE_RING_BUFFER_H

#include "evidence_descriptor.h"

#include <deque>
#include <map>
#include <string>
#include <vector>

namespace ns3 {

class EvidenceRingBuffer
{
public:
  EvidenceRingBuffer ();
  EvidenceRingBuffer (const std::string& nodeId, const std::string& nodeRole, uint32_t capacity);

  void Push (const EvidenceDescriptor& descriptor);
  void FreezeWindow (const std::string& eventId, uint64_t t0Ms, uint64_t preMs, uint64_t postMs);
  std::vector<EvidenceDescriptor> ExportFrozen (const std::string& eventId) const;
  uint64_t GetFrozenBytes (const std::string& eventId) const;

private:
  std::string m_nodeId;
  std::string m_nodeRole;
  uint32_t m_capacity;
  std::deque<EvidenceDescriptor> m_buffer;
  std::map<std::string, std::vector<EvidenceDescriptor> > m_frozen;
};

} // namespace ns3

#endif /* EVIDENCE_RING_BUFFER_H */
