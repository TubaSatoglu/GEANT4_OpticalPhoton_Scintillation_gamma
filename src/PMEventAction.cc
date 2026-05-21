#include "PMEventAction.hh"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"
#include "G4ios.hh"
#include "G4AnalysisManager.hh"

#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <string>

// -----------------------------------------------------------
// Static definitions
// -----------------------------------------------------------
int    PMEventAction::fEventSumPhotons  = 0;
int    PMEventAction::fEventSumDetected = 0;
int    PMEventAction::fEventCount       = 0;
double PMEventAction::fCurrentTheta     = 0.0;
std::ofstream PMEventAction::fCsvFile;
bool   PMEventAction::fCsvHeaderWritten = false;

std::vector<double> PMEventAction::sSumEmissGamma   (PM_PULSE_NBINS, 0.0);
std::vector<double> PMEventAction::sSumEmissNeutron (PM_PULSE_NBINS, 0.0);
std::vector<double> PMEventAction::sSumArrivGamma   (PM_PULSE_NBINS, 0.0);
std::vector<double> PMEventAction::sSumArrivNeutron (PM_PULSE_NBINS, 0.0);
int PMEventAction::sNGammaEvents   = 0;
int PMEventAction::sNNeutronEvents = 0;


// -----------------------------------------------------------
// Constructor
// -----------------------------------------------------------
PMEventAction::PMEventAction()
: G4UserEventAction(),
  fOpticalPhotonCount(0),
  fGammaTeflonCount(0),
  fAluminumPhotonCount(0),
  fPrimaryOpticalPhotonCount(0),
  fPhotonsAtAluminumBoundary(0),
  fScintillationCount(0),
  fTotalEnergyDep(0.0),
  fTailStartBin(static_cast<int>(2.0 / PM_BIN_WIDTH_NS)), // Anything after 2 ns is considered the tail
  fPrimaryPDG(0),
  fEmissGamma(PM_PULSE_NBINS, 0.0),
  fEmissNeutron(PM_PULSE_NBINS, 0.0),
  fArrivGamma(PM_PULSE_NBINS, 0.0),
  fArrivNeutron(PM_PULSE_NBINS, 0.0),
  fArrivalHist(PM_NBINS, 0.0),
  fEmissionHist(PM_NBINS, 0.0)
{
  fEdepTimeHist.assign(PM_NBINS, 0.0);
}

PMEventAction::~PMEventAction() = default;


// -----------------------------------------------------------
// Helpers
// -----------------------------------------------------------
void PMEventAction::ResetCounters()
{
  fOpticalPhotonCount         = 0;
  fGammaTeflonCount           = 0;
  fAluminumPhotonCount        = 0;
  fPrimaryOpticalPhotonCount  = 0;
  fPhotonsAtAluminumBoundary  = 0;

  fScintillationCount         = 0;
  fTotalEnergyDep             = 0.0;

  fEnergyDeposits.clear();
}

void PMEventAction::AddOpticalPhoton()        { ++fOpticalPhotonCount; }
void PMEventAction::AddPrimaryOpticalPhoton() { ++fPrimaryOpticalPhotonCount; }
void PMEventAction::AddGammaToTeflon()        { ++fGammaTeflonCount; }
void PMEventAction::AddAluminumPhoton()       { ++fAluminumPhotonCount; ++fPhotonsAtAluminumBoundary; }
void PMEventAction::AddScintillationPhoton()  { ++fPrimaryOpticalPhotonCount; }
void PMEventAction::RecordEnergy(double e)    { fTotalEnergyDep += e; fEnergyDeposits.push_back(e); }


// -----------------------------------------------------------
// Time-binned records
// -----------------------------------------------------------
void PMEventAction::RecordEdepTime(double time_ns, double edep)
{
  const std::size_t bin = static_cast<std::size_t>(time_ns / PM_BIN_WIDTH_NS);
  if (bin < fEdepTimeHist.size())
      fEdepTimeHist[bin] += edep; // MeV
}

void PMEventAction::RecordPhotonArrival(double time_ns)
{
  // Store as a list
  fPhotonArrivalT.push_back(time_ns);

  // Intra-event arrival histogram (for CSV output)
  AccumulateArrivalBin(time_ns);

  // For calculating average gamma / neutron pulses
  int bin = PulseTimeToBin(time_ns);
  if (fPrimaryPDG == 22) {
    fArrivGamma[bin] += 1.0;
  } else if (fPrimaryPDG == 2112) {
    fArrivNeutron[bin] += 1.0;
  }

  // Optional: You can also fill the AnalysisManager histogram here
  // auto am = G4AnalysisManager::Instance();
  // if (am) am->FillH1(PULSE_HISTO_ID, time_ns);
}

void PMEventAction::AccumulateArrivalBin(double t_ns)
{
  const std::size_t b = static_cast<std::size_t>(t_ns / PM_BIN_WIDTH_NS);
  if (b < fArrivalHist.size())
      fArrivalHist[b] += 1.0;
}

void PMEventAction::AccumulateEmissionBin(double t_ns)
{
  const std::size_t b = static_cast<std::size_t>(t_ns / PM_BIN_WIDTH_NS);
  if (b < fEmissionHist.size())
      fEmissionHist[b] += 1.0;
}


// -----------------------------------------------------------
// Record by primary (gamma / neutron identification)
// -----------------------------------------------------------
void PMEventAction::RecordEdepTimeByPrimary(double time_ns, double edep)
{
  const std::size_t bin = static_cast<std::size_t>(time_ns / PM_BIN_WIDTH_NS);
  if (bin >= PM_NBINS) return;

  const char* key =
    (fPrimaryPDG == 22)   ? "gamma_primary" :
    (fPrimaryPDG == 2112) ? "neutron_primary" :
                            "other_primary";

  ParticleInfo& info = fParticleData[key];
  info.edepHist[bin] += edep;
  info.totalEdep     += edep;
  info.totalCount    += 1;
}


// -----------------------------------------------------------
void PMEventAction::RecordEdepTimeByParticle(const G4String& name,
                                             double time_ns,
                                             double edep)
{
  if (edep < PM_EDEP_THRESHOLD_KEV * keV) return;

  const std::size_t bin = static_cast<std::size_t>(time_ns / PM_BIN_WIDTH_NS);
  if (bin >= PM_NBINS) return;

  ParticleInfo& info = fParticleData[name];
  info.edepHist[bin] += edep;
  info.totalEdep     += edep;
}

void PMEventAction::RecordParticleCount(const G4String& name)
{
  fParticleData[name].totalCount++;
}

void PMEventAction::RecordParticleInfo(const G4String& name,
                                       int trackID,
                                       int parentID,
                                       const G4String& process)
{
  ParticleInfo& info = fParticleData[name];
  info.trackIDs.insert(trackID);
  info.parentIDs.insert(parentID);
  info.processes.insert(process);
}


// -----------------------------------------------------------
// Pulse-shape calculation
// -----------------------------------------------------------
void PMEventAction::CalculatePulseFeatures(ParticleInfo& info)
{
  info.integral      = 0.0;
  info.tailIntegral = 0.0;
  info.peakValue    = 0.0;
  info.peakTime      = 0.0;
  info.meanTime      = 0.0;

  double wsum = 0.0;

  for (std::size_t i = 0; i < info.edepHist.size(); ++i)
  {
    const double v = info.edepHist[i];

    info.integral += v;
    if (i >= static_cast<std::size_t>(fTailStartBin))
        info.tailIntegral += v;

    if (v > info.peakValue) {
      info.peakValue = v;
      info.peakTime  = i * PM_BIN_WIDTH_NS;
    }

    wsum += v * i * PM_BIN_WIDTH_NS;
  }

  if (info.integral > 0.0)
      info.meanTime = wsum / info.integral;
}


// -----------------------------------------------------------
// CSV writer
// -----------------------------------------------------------
void PMEventAction::WriteCSVOutput(int eventID)
{
  bool allZero = std::all_of(
      fArrivalHist.begin(), fArrivalHist.end(),
      [](double v){ return v == 0.0; });

  // Do not write a row if there are no histogram entries and no particle data
  if (allZero && fParticleData.empty())
      return;

  // Header (Write only once)
  if (!fCsvHeaderWritten)
  {
    fCsvFile.open(PM_CSV_PATH, std::ios::out);
    fCsvFile
      << "EventID,Theta_deg,HistogramType,ParticleName,ParticleCount,"
      << "TotalOpticalPhoton,DetectedAtAl,PrimaryOpticalPhoton,"
      << "TotalEdep_MeV,PeakTime_ns,PeakValue,MeanTime_ns,"
      << "Integral,TailIntegral,TailRatio,"
      << "NumTracks,NumParents,CreatorProcesses";

    for (int i = 0; i < PM_NBINS; ++i)
        fCsvFile << ",TimeBin_" << i;

    fCsvFile << "\n";
    fCsvFile.close();
    fCsvHeaderWritten = true;
  }

  std::ofstream out(PM_CSV_PATH, std::ios::app);

  //---------------------------------------------------------
  // 1) Optical Arrival (Arrival at Al surface: THIS IS YOUR MAIN PULSE)
  //---------------------------------------------------------
  {
    double integral = 0, tail = 0, peakVal = 0, peakT = 0, wsum = 0;

    for (int i = 0; i < PM_NBINS; ++i)
    {
      double v = fArrivalHist[i];
      integral += v;
      if (i >= fTailStartBin) tail += v;
      if (v > peakVal) { peakVal = v; peakT = i * PM_BIN_WIDTH_NS; }
      wsum += v * i * PM_BIN_WIDTH_NS;
    }

    double meanT      = (integral > 0 ? wsum / integral : 0);
    double tailRatio = (integral > 0 ? tail / integral : 0);

    out << eventID << ","
        << std::fixed << std::setprecision(2) << GetCurrentTheta() << ","
        << "optical_arrival" << ","
        << PrimaryNameOf(fPrimaryPDG) << ","
        << static_cast<int>(integral) << ","
        << fOpticalPhotonCount << ","
        << fAluminumPhotonCount << ","
        << fPrimaryOpticalPhotonCount << ","
        << std::fixed << std::setprecision(6) << fTotalEnergyDep/MeV << ","
        << std::fixed << std::setprecision(3) << peakT << ","
        << std::scientific << peakVal << ","
        << std::fixed << std::setprecision(3) << meanT << ","
        << std::scientific << integral << ","
        << std::scientific << tail << ","
        << std::fixed << std::setprecision(4) << tailRatio << ","
        << 0 << "," << 0 << ","
        << "optical_arrival";

    for (double v : fArrivalHist)
        out << "," << std::scientific << v;

    out << "\n";
  }

  //---------------------------------------------------------
  // 2) Optical Emission (Optical photon generation times)
  //---------------------------------------------------------
  {
    double integralE = 0, tailE = 0, peakValE = 0, peakTE = 0, wsumE = 0;

    for (int i = 0; i < PM_NBINS; ++i)
    {
      double v = fEmissionHist[i];
      integralE += v;
      if (i >= fTailStartBin) tailE += v;
      if (v > peakValE) { peakValE = v; peakTE = i * PM_BIN_WIDTH_NS; }
      wsumE += v * i * PM_BIN_WIDTH_NS;
    }

    double meanTE     = (integralE > 0 ? wsumE / integralE : 0);
    double tailRatioE = (integralE > 0 ? tailE / integralE : 0);

    out << eventID << ","
        << std::fixed << std::setprecision(2) << GetCurrentTheta() << ","
        << "optical_emission" << ","
        << PrimaryNameOf(fPrimaryPDG) << ","
        << static_cast<int>(integralE) << ","
        << fOpticalPhotonCount << ","
        << fAluminumPhotonCount << ","
        << fPrimaryOpticalPhotonCount << ","
        << std::fixed << std::setprecision(6) << fTotalEnergyDep/MeV << ","
        << std::fixed << std::setprecision(3) << peakTE << ","
        << std::scientific << peakValE << ","
        << std::fixed << std::setprecision(3) << meanTE << ","
        << std::scientific << integralE << ","
        << std::scientific << tailE << ","
        << std::fixed << std::setprecision(4) << tailRatioE << ","
        << 0 << "," << 0 << ","
        << "optical_emission";

    for (double v : fEmissionHist)
        out << "," << std::scientific << v;

    out << "\n";
  }

  //---------------------------------------------------------
  // 3) Edep vs time (Total energy deposition vs time)
  //---------------------------------------------------------
  {
    double integralEd = 0, tailEd = 0, peakValEd = 0, peakTEd = 0, wsumEd = 0;

    for (int i = 0; i < PM_NBINS; ++i)
    {
      double v = fEdepTimeHist[i]; // MeV/bin
      integralEd += v;
      if (i >= fTailStartBin) tailEd += v;
      if (v > peakValEd) { peakValEd = v; peakTEd = i * PM_BIN_WIDTH_NS; }
      wsumEd += v * i * PM_BIN_WIDTH_NS;
    }

    double meanTEd     = (integralEd > 0 ? wsumEd / integralEd : 0);
    double tailRatioEd = (integralEd > 0 ? tailEd / integralEd : 0);

    out << eventID << ","
        << std::fixed << std::setprecision(2) << GetCurrentTheta() << ","
        << "edep_time" << ","
        << PrimaryNameOf(fPrimaryPDG) << ","
        << 0 << ","                         // ParticleCount is not meaningful here
        << fOpticalPhotonCount << ","
        << fAluminumPhotonCount << ","
        << fPrimaryOpticalPhotonCount << ","
        << std::fixed << std::setprecision(6) << fTotalEnergyDep/MeV << ","
        << std::fixed << std::setprecision(3) << peakTEd << ","
        << std::scientific << peakValEd << ","
        << std::fixed << std::setprecision(3) << meanTEd << ","
        << std::scientific << integralEd << ","
        << std::scientific << tailEd << ","
        << std::fixed << std::setprecision(4) << tailRatioEd << ","
        << 0 << "," << 0 << ","
        << "edep_time";

    for (double v : fEdepTimeHist)
        out << "," << std::scientific << v; // MeV per bin

    out << "\n";
  }

  //---------------------------------------------------------
  // 4) Particle-based EDEP histograms ("edep")
  //---------------------------------------------------------
  for (auto& kv : fParticleData)
  {
    const G4String& name = kv.first;
    ParticleInfo&   info = kv.second;

    CalculatePulseFeatures(info);
    double tailRatio2 = (info.integral > 0 ? info.tailIntegral / info.integral : 0);

    std::string proc;
    for (const auto& p : info.processes)
    {
      if (!proc.empty()) proc += "; ";
      proc += p;
    }

    out << eventID << ","
        << std::fixed << std::setprecision(2) << GetCurrentTheta() << ","
        << "edep" << ","
        << name << ","
        << info.totalCount << ","
        << fOpticalPhotonCount << ","
        << fAluminumPhotonCount << ","
        << fPrimaryOpticalPhotonCount << ","
        << std::fixed << std::setprecision(6) << info.totalEdep/MeV << ","
        << std::fixed << std::setprecision(3) << info.peakTime << ","
        << std::scientific << info.peakValue << ","
        << std::fixed << std::setprecision(3) << info.meanTime << ","
        << std::scientific << info.integral << ","
        << std::scientific << info.tailIntegral << ","
        << std::fixed << std::setprecision(4) << tailRatio2 << ","
        << info.trackIDs.size() << ","
        << info.parentIDs.size() << ","
        << proc;

    for (double v : info.edepHist)
        out << "," << std::scientific << v;

    out << "\n";
  }

  out.close();
}


// -----------------------------------------------------------
// Event begin
// -----------------------------------------------------------
void PMEventAction::BeginOfEventAction(const G4Event*)
{
  ResetCounters();

  fEdepTimeHist.assign(PM_NBINS, 0.0);
  fArrivalHist.assign(PM_NBINS, 0.0);
  fEmissionHist.assign(PM_NBINS, 0.0);

  fPhotonArrivalT.clear();
  fParticleData.clear();

  fEmissGamma.assign(PM_PULSE_NBINS, 0.0);
  fEmissNeutron.assign(PM_PULSE_NBINS, 0.0);
  fArrivGamma.assign(PM_PULSE_NBINS, 0.0);
  fArrivNeutron.assign(PM_PULSE_NBINS, 0.0);

  fPrimaryPDG = 0;
}


// -----------------------------------------------------------
// Event end
// -----------------------------------------------------------
void PMEventAction::EndOfEventAction(const G4Event* evt)
{
  // Run-level statistics
  AccumulateRun(fPrimaryOpticalPhotonCount, fAluminumPhotonCount);

  const G4int eventID = evt ? evt->GetEventID() : -1;

  // ======= TERMINAL OUTPUT =======
  G4cout << "\n======= Event Summary =======" << G4endl;
  G4cout << "🆔 Event ID: " << eventID << G4endl;
  G4cout << "🔹 Total Optical Photons (all tracks): "
         << fOpticalPhotonCount << G4endl;
  G4cout << "🔹 Primary Optical Photons (Scintillation): "
         << fPrimaryOpticalPhotonCount << G4endl;
  G4cout << "🔹 Optical Photons Detected at Aluminum: "
         << fAluminumPhotonCount << G4endl;
  G4cout << "🔹 Total Edep in plastic (MeV): "
         << fTotalEnergyDep/MeV << G4endl;
  G4cout << "=====================================\n" << G4endl;
  // ===============================

  // Save to CSV
  WriteCSVOutput(eventID);

  // Separate average pulse accumulators for gamma and neutrons
  if (fPrimaryPDG == 22)
  {
    for (int i = 0; i < PM_PULSE_NBINS; ++i)
    {
      sSumEmissGamma[i] += fEmissGamma[i];
      sSumArrivGamma[i] += fArrivGamma[i];
    }
    ++sNGammaEvents;
  }
  else if (fPrimaryPDG == 2112)
  {
    for (int i = 0; i < PM_PULSE_NBINS; ++i)
    {
      sSumEmissNeutron[i] += fEmissNeutron[i];
      sSumArrivNeutron[i] += fArrivNeutron[i];
    }
    ++sNNeutronEvents;
  }
}


// -----------------------------------------------------------
// Run counters
// -----------------------------------------------------------
void PMEventAction::ResetRunCounters()
{
  fEventSumPhotons  = 0;
  fEventSumDetected = 0;
  fEventCount       = 0;
}

void PMEventAction::AccumulateRun(int photons, int detected)
{
  fEventSumPhotons  += photons;
  fEventSumDetected += detected;
  ++fEventCount;
}

double PMEventAction::GetRunAverage()
{
  return (fEventCount > 0 ? (double)fEventSumPhotons / fEventCount : 0.0);
}

int PMEventAction::GetRunTotal()    { return fEventSumPhotons; }
int PMEventAction::GetRunDetected() { return fEventSumDetected; }


// -----------------------------------------------------------
// Pulse-shape CSV (Average Pulses)
// -----------------------------------------------------------
void PMEventAction::SavePulseShapeCSVs()
{
  auto write_avg = [&](const std::string& fn,
                       const std::vector<double>& S,
                       int N)
  {
    std::ofstream f(fn);
    f << "time_ns,normalized_counts\n";

    if (N <= 0) return;

    const double dt = PM_PULSE_DT_NS;

    double area = 0.0;
    for (double v : S) area += v * dt;

    for (int i = 0; i < PM_PULSE_NBINS; ++i)
    {
      double val = S[i];
      val = (area > 0 ? val / area : 0);
      val /= N;

      f << std::fixed << std::setprecision(3) << i * dt << ","
        << std::setprecision(6) << val << "\n";
    }
  };

  write_avg("avg_emission_gamma.csv",   sSumEmissGamma,   sNGammaEvents);
  write_avg("avg_emission_neutron.csv", sSumEmissNeutron, sNNeutronEvents);
  write_avg("avg_output_gamma.csv",     sSumArrivGamma,   sNGammaEvents);
  write_avg("avg_output_neutron.csv",   sSumArrivNeutron, sNNeutronEvents);
}


// -----------------------------------------------------------
// Backwards compatibility wrappers
// -----------------------------------------------------------
void PMEventAction::AccumulateEmissionTime(double t_ns)
{
  AccumulateEmissionBin(t_ns);

  int bin = PulseTimeToBin(t_ns);
  if (fPrimaryPDG == 22) {
    fEmissGamma[bin] += 1.0;
  }
  else if (fPrimaryPDG == 2112) {
    fEmissNeutron[bin] += 1.0;
  }
}

void PMEventAction::AccumulateArrivalTime(double t_ns)
{
  AccumulateArrivalBin(t_ns);
}
