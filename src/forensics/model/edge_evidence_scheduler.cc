/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#include "edge_evidence_scheduler.h"

#include <algorithm>

namespace ns3 {

namespace {

bool
SortByTimestamp (const EvidenceDescriptor& left, const EvidenceDescriptor& right)
{
  if (left.GetTimestampMs () == right.GetTimestampMs ())
    {
      return left.GetEvidenceId () < right.GetEvidenceId ();
    }
  return left.GetTimestampMs () < right.GetTimestampMs ();
}

bool
SortByDeadline (const EvidenceDescriptor& left, const EvidenceDescriptor& right)
{
  if (left.GetDeadlineMs () == right.GetDeadlineMs ())
    {
      return SortByTimestamp (left, right);
    }
  return left.GetDeadlineMs () < right.GetDeadlineMs ();
}

bool
SortByCostDeadline (const EvidenceDescriptor& left, const EvidenceDescriptor& right)
{
  if (left.GetCost () == right.GetCost ())
    {
      return SortByDeadline (left, right);
    }
  return left.GetCost () < right.GetCost ();
}

} // namespace

EdgeEvidenceScheduler::EdgeEvidenceScheduler ()
  : m_mode ("proposed"),
    m_uploadBudgetBytes (1500),
    m_w1 (0.35),
    m_w2 (0.25),
    m_w3 (0.15),
    m_w4 (0.15),
    m_w5 (0.10),
    m_serviceReserveRatio (0.35)
{
}

void EdgeEvidenceScheduler::SetMode (const std::string& mode) { m_mode = mode; }
void EdgeEvidenceScheduler::SetUploadBudgetBytes (uint64_t budgetBytes) { m_uploadBudgetBytes = budgetBytes; }

void
EdgeEvidenceScheduler::SetWeights (double w1, double w2, double w3, double w4, double w5)
{
  m_w1 = w1;
  m_w2 = w2;
  m_w3 = w3;
  m_w4 = w4;
  m_w5 = w5;
}

void
EdgeEvidenceScheduler::SetServiceReserveRatio (double ratio)
{
  m_serviceReserveRatio = EvidenceDescriptor::Clamp01 (ratio);
}

std::vector<EvidenceDescriptor>
EdgeEvidenceScheduler::Select (const std::vector<EvidenceDescriptor>& descriptors, uint64_t baselineServiceBytes) const
{
  std::vector<EvidenceDescriptor> ranked = descriptors;
  uint64_t effectiveBudget = m_uploadBudgetBytes;

  if (m_mode == "latency-first")
    {
      std::sort (ranked.begin (), ranked.end (), SortByDeadline);
    }
  else if (m_mode == "service-priority")
    {
      std::sort (ranked.begin (), ranked.end (), SortByCostDeadline);
      const double reserved = static_cast<double> (baselineServiceBytes) * m_serviceReserveRatio;
      if (reserved < static_cast<double> (effectiveBudget))
        {
          effectiveBudget -= static_cast<uint64_t> (reserved);
        }
      else
        {
          effectiveBudget = 0;
        }
    }
  else if (m_mode == "full-upload")
    {
      std::sort (ranked.begin (), ranked.end (), SortByTimestamp);
      effectiveBudget = 0;
      for (std::vector<EvidenceDescriptor>::const_iterator it = descriptors.begin (); it != descriptors.end (); ++it)
        {
          effectiveBudget += it->GetSizeBytes ();
        }
    }
  else
    {
      std::sort (
        ranked.begin (),
        ranked.end (),
        [this] (const EvidenceDescriptor& left, const EvidenceDescriptor& right) {
          const double leftScore = Score (left);
          const double rightScore = Score (right);
          if (leftScore == rightScore)
            {
              return SortByDeadline (left, right);
            }
          return leftScore > rightScore;
        });
    }

  std::vector<EvidenceDescriptor> selected;
  uint64_t usedBytes = 0;
  for (std::vector<EvidenceDescriptor>::const_iterator it = ranked.begin (); it != ranked.end (); ++it)
    {
      if ((usedBytes + it->GetSizeBytes ()) > effectiveBudget)
        {
          continue;
        }
      selected.push_back (*it);
      usedBytes += it->GetSizeBytes ();
    }
  return selected;
}

double
EdgeEvidenceScheduler::Score (const EvidenceDescriptor& descriptor) const
{
  return descriptor.ComputeProposedScore (m_w1, m_w2, m_w3, m_w4, m_w5);
}

} // namespace ns3
