/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "evidence_descriptor.h"

namespace ns3 {

EvidenceDescriptor::EvidenceDescriptor ()
  : m_timestampMs (0),
    m_sizeBytes (0),
    m_deadlineMs (0),
    m_relevance (0.0),
    m_urgency (0.0),
    m_credibility (0.0),
    m_complementarity (0.0),
    m_cost (0.0),
    m_isKeyTarget (false)
{
}

EvidenceDescriptor::EvidenceDescriptor (const std::string& eventId,
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
                                        bool isKeyTarget)
  : m_eventId (eventId),
    m_evidenceId (evidenceId),
    m_sourceId (sourceId),
    m_nodeRole (nodeRole),
    m_type (type),
    m_timestampMs (timestampMs),
    m_sizeBytes (sizeBytes),
    m_deadlineMs (deadlineMs),
    m_relevance (Clamp01 (relevance)),
    m_urgency (Clamp01 (urgency)),
    m_credibility (Clamp01 (credibility)),
    m_complementarity (Clamp01 (complementarity)),
    m_cost (Clamp01 (cost)),
    m_isKeyTarget (isKeyTarget)
{
}

std::string EvidenceDescriptor::GetEventId () const { return m_eventId; }
std::string EvidenceDescriptor::GetEvidenceId () const { return m_evidenceId; }
std::string EvidenceDescriptor::GetSourceId () const { return m_sourceId; }
std::string EvidenceDescriptor::GetNodeRole () const { return m_nodeRole; }
std::string EvidenceDescriptor::GetType () const { return m_type; }
uint64_t EvidenceDescriptor::GetTimestampMs () const { return m_timestampMs; }
uint32_t EvidenceDescriptor::GetSizeBytes () const { return m_sizeBytes; }
uint64_t EvidenceDescriptor::GetDeadlineMs () const { return m_deadlineMs; }
double EvidenceDescriptor::GetRelevance () const { return m_relevance; }
double EvidenceDescriptor::GetUrgency () const { return m_urgency; }
double EvidenceDescriptor::GetCredibility () const { return m_credibility; }
double EvidenceDescriptor::GetComplementarity () const { return m_complementarity; }
double EvidenceDescriptor::GetCost () const { return m_cost; }
bool EvidenceDescriptor::IsKeyTarget () const { return m_isKeyTarget; }

double
EvidenceDescriptor::ComputeProposedScore (double w1, double w2, double w3, double w4, double w5) const
{
  return (w1 * m_relevance)
         + (w2 * m_urgency)
         + (w3 * m_credibility)
         + (w4 * m_complementarity)
         - (w5 * m_cost);
}

bool
EvidenceDescriptor::DeadlineSatisfied (uint64_t arrivalMs) const
{
  return arrivalMs <= m_deadlineMs;
}

double
EvidenceDescriptor::Clamp01 (double value)
{
  if (value < 0.0)
    {
      return 0.0;
    }
  if (value > 1.0)
    {
      return 1.0;
    }
  return value;
}

} // namespace ns3
