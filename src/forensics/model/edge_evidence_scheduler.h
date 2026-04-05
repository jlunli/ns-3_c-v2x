/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

#ifndef EDGE_EVIDENCE_SCHEDULER_H
#define EDGE_EVIDENCE_SCHEDULER_H

#include "evidence_descriptor.h"

#include <string>
#include <vector>

namespace ns3 {

class EdgeEvidenceScheduler
{
public:
  EdgeEvidenceScheduler ();

  void SetMode (const std::string& mode);
  void SetUploadBudgetBytes (uint64_t budgetBytes);
  void SetWeights (double w1, double w2, double w3, double w4, double w5);
  void SetServiceReserveRatio (double ratio);

  std::vector<EvidenceDescriptor> Select (const std::vector<EvidenceDescriptor>& descriptors,
                                          uint64_t baselineServiceBytes) const;
  double Score (const EvidenceDescriptor& descriptor) const;

private:
  std::string m_mode;
  uint64_t m_uploadBudgetBytes;
  double m_w1;
  double m_w2;
  double m_w3;
  double m_w4;
  double m_w5;
  double m_serviceReserveRatio;
};

} // namespace ns3

#endif /* EDGE_EVIDENCE_SCHEDULER_H */
