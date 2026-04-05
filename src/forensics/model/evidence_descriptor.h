/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef EVIDENCE_DESCRIPTOR_H
#define EVIDENCE_DESCRIPTOR_H

#include <stdint.h>
#include <string>

namespace ns3 {

class EvidenceDescriptor
{
public:
  EvidenceDescriptor ();
  EvidenceDescriptor (const std::string& eventId,
                      const std::string& evidenceId,
                      const std::string& sourceId,
                      const std::string& nodeRole,
                      const std::string& type,
                      uint64_t timestampMs,
                      uint32_t sizeBytes,
                      uint64_t deadlineMs,
                      double relevance,
                      double urgency,
                      double credibility,
                      double complementarity,
                      double cost,
                      bool isKeyTarget);

  std::string GetEventId () const;
  std::string GetEvidenceId () const;
  std::string GetSourceId () const;
  std::string GetNodeRole () const;
  std::string GetType () const;
  uint64_t GetTimestampMs () const;
  uint32_t GetSizeBytes () const;
  uint64_t GetDeadlineMs () const;
  double GetRelevance () const;
  double GetUrgency () const;
  double GetCredibility () const;
  double GetComplementarity () const;
  double GetCost () const;
  bool IsKeyTarget () const;

  double ComputeProposedScore (double w1, double w2, double w3, double w4, double w5) const;
  bool DeadlineSatisfied (uint64_t arrivalMs) const;

  static double Clamp01 (double value);

private:
  std::string m_eventId;
  std::string m_evidenceId;
  std::string m_sourceId;
  std::string m_nodeRole;
  std::string m_type;
  uint64_t m_timestampMs;
  uint32_t m_sizeBytes;
  uint64_t m_deadlineMs;
  double m_relevance;
  double m_urgency;
  double m_credibility;
  double m_complementarity;
  double m_cost;
  bool m_isKeyTarget;
};

} // namespace ns3

#endif /* EVIDENCE_DESCRIPTOR_H */
