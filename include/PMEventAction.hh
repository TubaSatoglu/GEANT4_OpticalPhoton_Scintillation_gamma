#ifndef PM_EVENT_ACTION_HH
#define PM_EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "G4String.hh"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <string>

// Time bin parameters
static constexpr int    PM_NBINS        = 5000;   // 0..4999
static constexpr double PM_BIN_WIDTH_NS = 0.1;    // [ns]

// Edep threshold (for time-binned energy recording)
static constexpr double PM_EDEP_THRESHOLD_KEV = 0.001; // 1e-3 keV

// Pulse-shape average histogram parameters
static constexpr int    PM_PULSE_NBINS  = 5000;
static constexpr double PM_PULSE_DT_NS  = 0.1;    // [ns]

static constexpr const char* PM_CSV_PATH =
    "/home/tuba/Documents/geant4_examples/PSD_data(GAMMA)(E=3MeV)(N=5000).csv";

class G4Event;

class PMEventAction : public G4UserEventAction {
public:
  PMEventAction();
  ~PMEventAction() override;

  void BeginOfEventAction(const G4Event*) override;
  void EndOfEventAction  (const G4Event*) override;

  inline void SetPrimaryPDG(int pdg) { fPrimaryPDG = pdg; }

  // Counters / recorders
  void AddOpticalPhoton();
  void AddPrimaryOpticalPhoton();
  void CountPrimaryPhoton() { AddPrimaryOpticalPhoton(); }
  void AddGammaToTeflon();
  void AddAluminumPhoton();
  void AddScintillationPhoton();
  void RecordEnergy(double e); // MeV

  // Time-binning functions
  void RecordEdepTime(double time_ns, double edep);
  void RecordPhotonArrival(double time_ns); // Arrival at Al surface
  void RecordEdepTimeByParticle(const G4String& particleName,
                                double time_ns, double edep);
  void RecordEdepTimeByPrimary(double time_ns, double edep);

  void RecordParticleCount(const G4String& particleName);
  void RecordParticleInfo(const G4String& particleName,
                          int trackID,
                          int parentID,
                          const G4String& creatorProcess);

  void AccumulateEmissionTime(double t_ns); // Optical photon emission time
  void AccumulateArrivalTime (double t_ns); // For backward compatibility

  void AccumulateArrivalBin (double t_ns);
  void AccumulateEmissionBin(double t_ns);

  // Run-summary counters
  static void   ResetRunCounters();
  static void   AccumulateRun(int photons, int detected);
  static double GetRunAverage();
  static int    GetRunTotal();
  static int    GetRunDetected();

  static inline void   SetCurrentTheta(double th) { fCurrentTheta = th; }
  static inline double GetCurrentTheta()          { return fCurrentTheta; }

  void SavePulseShapeCSVs();

  // --- Getters (for terminal output or SD) ---
  inline int    GetOpticalPhotonCount()        const { return fOpticalPhotonCount; }
  inline int    GetPrimaryOpticalPhotonCount() const { return fPrimaryOpticalPhotonCount; }
  inline int    GetAluminumPhotonCount()       const { return fAluminumPhotonCount; }
  inline double GetTotalEnergyDepMeV()         const { return fTotalEnergyDep; }

private:
  struct ParticleInfo {
    int                totalCount = 0;
    double             totalEdep  = 0.0;                                 // MeV
    std::vector<double> edepHist   = std::vector<double>(PM_NBINS, 0.0); // MeV/bin

    double integral      = 0.0; // MeV
    double tailIntegral = 0.0; // MeV
    double peakValue    = 0.0; // MeV/bin
    double peakTime      = 0.0; // ns
    double meanTime      = 0.0; // ns

    std::set<int>      trackIDs;
    std::set<int>      parentIDs;
    std::set<G4String> processes;
  };

  void ResetCounters();
  void WriteCSVOutput(int eventID);
  void CalculatePulseFeatures(ParticleInfo& info);

  static inline int PulseTimeToBin(double t_ns) {
    int b = static_cast<int>(t_ns / PM_PULSE_DT_NS);
    if (b < 0) b = 0;
    if (b >= PM_PULSE_NBINS) b = PM_PULSE_NBINS - 1;
    return b;
  }

  // Generate primary name (for CSV)
  inline static std::string PrimaryNameOf(int pdg) {
    switch (pdg) {
      case 22:   return "gamma";
      case 2112: return "neutron";
      case 11:   return "e-";
      case -11:  return "e+";
      case 2212: return "proton";
      default:   return "unknown";
    }
  }

private:
  // Intra-event counters
  int    fOpticalPhotonCount;
  int    fGammaTeflonCount;
  int    fAluminumPhotonCount;
  int    fPrimaryOpticalPhotonCount;
  int    fPhotonsAtAluminumBoundary;
  int    fScintillationCount;

  double              fTotalEnergyDep;     // MeV
  std::vector<double> fEnergyDeposits;     // MeV (step-by-step)
  std::vector<double> fEdepTimeHist;       // MeV/bin
  std::vector<double> fPhotonArrivalT;     // ns (list)

  int  fTailStartBin;
  int  fPrimaryPDG;

  // Pulse-shape averages (run level)
  static std::vector<double> sSumEmissGamma;
  static std::vector<double> sSumEmissNeutron;
  static std::vector<double> sSumArrivGamma;
  static std::vector<double> sSumArrivNeutron;
  static int sNGammaEvents;
  static int sNNeutronEvents;

  // Intra-event accumulators (gamma/neutron separated)
  std::vector<double> fEmissGamma;
  std::vector<double> fEmissNeutron;
  std::vector<double> fArrivGamma;
  std::vector<double> fArrivNeutron;

  // Intra-event histograms (raw)
  std::vector<double> fArrivalHist;   // Optical arrival count (count/bin)
  std::vector<double> fEmissionHist;  // Optical emission count (count/bin)

  // Particle-based energy-time data
  std::map<G4String, ParticleInfo> fParticleData;

  // Run summary counters
  static int    fEventSumPhotons;
  static int    fEventSumDetected;
  static int    fEventCount;
  static double fCurrentTheta;

  // CSV stream
  static std::ofstream fCsvFile;
  static bool          fCsvHeaderWritten;
};

#endif // PM_EVENT_ACTION_HH
