#include "PMRunAction.hh"
#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include <sstream>
#include <cstdio>
#include <iomanip>

PMRunAction::PMRunAction(G4double energy)
  : G4UserRunAction(),
    fEnergy(energy)
{
    auto* analysisManager = G4AnalysisManager::Instance();
    analysisManager->SetVerboseLevel(1);

    analysisManager->CreateH1("Edep", "Energy deposit", 100, 0., 10 * MeV);
    analysisManager->CreateH1("OpticalPhotons", "Optical Photon Count per Event", 100, 0, 500);
    analysisManager->CreateH1("GammaTeflon", "Gammas at Teflon Barrier per Event", 100, 0, 100);

    analysisManager->CreateNtuple("Photons", "Optical Photons");
    analysisManager->CreateNtupleIColumn("iEvent");
    analysisManager->CreateNtupleDColumn("fX");
    analysisManager->CreateNtupleDColumn("fY");
    analysisManager->CreateNtupleDColumn("fZ");
    analysisManager->CreateNtupleDColumn("fGlobalTime");
    analysisManager->CreateNtupleDColumn("fWlen");
    analysisManager->CreateNtupleDColumn("Edep");
    analysisManager->FinishNtuple();


    analysisManager->CreateNtuple("Waves", "Waveform data");
    analysisManager->CreateNtupleIColumn(1, "eventID");
    analysisManager->CreateNtupleDColumn(1, "totalE");
    analysisManager->CreateNtupleDColumn(1, "tt_ratio");
    analysisManager->CreateNtupleDColumn(1, "totalE_record");
    
    // Waveform columns for 2000 time bins
    for (int i = 0; i < 2000; i++) {
        std::ostringstream colName;
        colName << "h" << std::setw(3) << std::setfill('0') << i;
        analysisManager->CreateNtupleDColumn(1, colName.str());
    }
    analysisManager->FinishNtuple(1);    
}

PMRunAction::~PMRunAction() {}

void PMRunAction::BeginOfRunAction(const G4Run*)
{
    auto* analysisManager = G4AnalysisManager::Instance();

    // (Optional) Reset left-overs from previous runs for visualization purposes
    analysisManager->Reset();

    std::stringstream filename;
    filename << "simulation_output_time_domain" << fEnergy << "MeV.root";

#ifdef G4MULTITHREADED
    analysisManager->SetNtupleMerging(true);
#endif

    analysisManager->SetFileName(filename.str());
    analysisManager->OpenFile();

    G4cout << "Run started with " << fEnergy << " MeV. Data will be saved in: "
           << filename.str() << G4endl;
}

void PMRunAction::EndOfRunAction(const G4Run*)
{
    auto* analysisManager = G4AnalysisManager::Instance();
    if (analysisManager->IsActive()) {
        analysisManager->Write();
        analysisManager->CloseFile(false); // <<=== Keep in RAM for visualization purposes
    }
    G4cout << "Run finished. Data saved in: simulation_output_time_domain"
           << fEnergy << "MeV.root" << G4endl;
}
