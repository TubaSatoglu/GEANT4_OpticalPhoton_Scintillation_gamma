#ifndef PM_EVENT_ACTION_HH
#define PM_EVENT_ACTION_HH

#include "G4UserEventAction.hh"
#include "G4String.hh"
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <string>

// Zaman bin parametreleri
static constexpr int    PM_NBINS        = 5000;   // 0..4999
static constexpr double PM_BIN_WIDTH_NS = 0.1;    // [ns]

// Edep threshold (time-binned enerji kaydı için)
static constexpr double PM_EDEP_THRESHOLD_KEV = 0.001; // 1e-3 keV

// Pulse-shape ortalama histogram parametreleri
static constexpr int    PM_PULSE_NBINS  = 5000;
static constexpr double PM_PULSE_DT_NS  = 0.1;    // [ns]

// CSV çıktı yolu
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

  // Sayaçlar / kayıtçılar
  void AddOpticalPhoton();
  void AddPrimaryOpticalPhoton();
  void CountPrimaryPhoton() { AddPrimaryOpticalPhoton(); }
  void AddGammaToTeflon();
  void AddAluminumPhoton();
  void AddScintillationPhoton();
  void RecordEnergy(double e); // MeV

  // Zaman-binning fonksiyonları
  void RecordEdepTime(double time_ns, double edep);
  void RecordPhotonArrival(double time_ns); // Al yüzeyine varış
  void RecordEdepTimeByParticle(const G4String& particleName,
                                double time_ns, double edep);
  void RecordEdepTimeByPrimary(double time_ns, double edep);

  void RecordParticleCount(const G4String& particleName);
  void RecordParticleInfo(const G4String& particleName,
                          int trackID,
                          int parentID,
                          const G4String& creatorProcess);

  void AccumulateEmissionTime(double t_ns); // optik foton üretim zamanı
  void AccumulateArrivalTime (double t_ns); // eski isimle uyum için

  void AccumulateArrivalBin (double t_ns);
  void AccumulateEmissionBin(double t_ns);

  // Run-özet sayaçları
  static void   ResetRunCounters();
  static void   AccumulateRun(int photons, int detected);
  static double GetRunAverage();
  static int    GetRunTotal();
  static int    GetRunDetected();

  static inline void   SetCurrentTheta(double th) { fCurrentTheta = th; }
  static inline double GetCurrentTheta()          { return fCurrentTheta; }

  void SavePulseShapeCSVs();

  // --- Getter'lar (terminal output veya SD için) ---
  inline int    GetOpticalPhotonCount()        const { return fOpticalPhotonCount; }
  inline int    GetPrimaryOpticalPhotonCount() const { return fPrimaryOpticalPhotonCount; }
  inline int    GetAluminumPhotonCount()       const { return fAluminumPhotonCount; }
  inline double GetTotalEnergyDepMeV()         const { return fTotalEnergyDep; }

private:
  struct ParticleInfo {
    int                 totalCount = 0;
    double              totalEdep  = 0.0;                         // MeV
    std::vector<double> edepHist   = std::vector<double>(PM_NBINS, 0.0); // MeV/bin

    double integral     = 0.0; // MeV
    double tailIntegral = 0.0; // MeV
    double peakValue    = 0.0; // MeV/bin
    double peakTime     = 0.0; // ns
    double meanTime     = 0.0; // ns

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

  // Primary adını üret (CSV için)
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
  // Event içi sayaçlar
  int    fOpticalPhotonCount;
  int    fGammaTeflonCount;
  int    fAluminumPhotonCount;
  int    fPrimaryOpticalPhotonCount;
  int    fPhotonsAtAluminumBoundary;
  int    fScintillationCount;

  double              fTotalEnergyDep;     // MeV
  std::vector<double> fEnergyDeposits;     // MeV (step-step)
  std::vector<double> fEdepTimeHist;       // MeV/bin
  std::vector<double> fPhotonArrivalT;     // ns (liste)

  int  fTailStartBin;
  int  fPrimaryPDG;

  // Pulse-shape ortalamaları (run seviyesi)
  static std::vector<double> sSumEmissGamma;
  static std::vector<double> sSumEmissNeutron;
  static std::vector<double> sSumArrivGamma;
  static std::vector<double> sSumArrivNeutron;
  static int sNGammaEvents;
  static int sNNeutronEvents;

  // Event içi akümülatörler (gamma/neutron ayırımlı)
  std::vector<double> fEmissGamma;
  std::vector<double> fEmissNeutron;
  std::vector<double> fArrivGamma;
  std::vector<double> fArrivNeutron;

  // Event içi histogramlar (raw)
  std::vector<double> fArrivalHist;   // optik varış sayımı (count/bin)
  std::vector<double> fEmissionHist;  // optik üretim sayımı (count/bin)

  // Parçacık bazlı enerji-zaman verisi
  std::map<G4String, ParticleInfo> fParticleData;

  // Run özet sayaçları
  static int    fEventSumPhotons;
  static int    fEventSumDetected;
  static int    fEventCount;
  static double fCurrentTheta;

  // CSV stream
  static std::ofstream fCsvFile;
  static bool          fCsvHeaderWritten;
};

#endif // PM_EVENT_ACTION_HH
