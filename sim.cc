// sim.cc
#include "G4RunManager.hh"
#ifdef G4MULTITHREADED
  #include "G4MTRunManager.hh"
#endif
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"
#include "G4UIExecutive.hh"
#include "PMDetectorConstruction.hh"
#include "PMPhysicsList.hh"
#include "PMActionInitialization.hh"
#include "PMPrimaryGenerator.hh"
#include "PMEventAction.hh"
#include "G4SystemOfUnits.hh"
#include "G4Threading.hh"
#include "CLHEP/Random/Random.h"

#include <fstream>
#include <string>
#include <ctime>
#include <random>
#include <algorithm> // std::max

#ifdef G4MULTITHREADED
using RunManager_t = G4MTRunManager;
#else
using RunManager_t = G4RunManager;
#endif

int main(int argc, char** argv) {
    // === RANDOM SEED ===
    G4long seed = time(nullptr);
    CLHEP::HepRandom::setTheSeed(seed);
    G4cout << "🔀 Random seed set to: " << seed << G4endl;

    // Is UI available?
    G4UIExecutive* ui = (argc == 1) ? new G4UIExecutive(argc, argv) : nullptr;

    // Create RunManager (of type G4MTRunManager if MT is enabled)
    auto* runManager = new RunManager_t();

#ifdef G4MULTITHREADED
    static_cast<G4MTRunManager*>(runManager)->SetNumberOfThreads(1);
#endif

    runManager->SetUserInitialization(new PMDetectorConstruction());
    runManager->SetUserInitialization(new PMPhysicsList());

    G4VisManager* visManager = new G4VisExecutive;
    visManager->Initialize();

    const int N = 5000;   // Number of events

    for (int E_MeV = 3; E_MeV <= 3; ++E_MeV) {
        G4cout << "\n=== Starting simulations for Gamma " << E_MeV << " MeV ===" << G4endl;

        auto* actionInit = new PMActionInitialization(E_MeV * MeV);
        runManager->SetUserInitialization(actionInit);
        runManager->Initialize();

        std::string filename = "/home/tuba/Documents/gamma_results(timebin deneme)_" + std::to_string(E_MeV) + "MeV.csv";
        std::ofstream fout(filename);
        fout << "theta_deg,  total_optical_photons" << std::endl;

        auto* primaryGen = actionInit->GetPrimaryGenerator();

        for (int theta = 90; theta <= 90; theta += 2) {
            primaryGen->SetBeamTheta(theta * deg);
            PMEventAction::SetCurrentTheta(theta);
            PMEventAction::ResetRunCounters(); // ⚠️ Remember to switch to atomic/accumulable in MT mode

            runManager->BeamOn(N);

            int totalPhotons  = PMEventAction::GetRunTotal();
            int totalDetected = PMEventAction::GetRunDetected();

            //fout << theta << ",  " << totalPhotons << std::endl;
            //fout << theta << ",  " << totalPhotons << ",  " << totalDetected << std::endl;

            G4cout << " ✅ E=" << E_MeV
                   << " MeV, "
                   << "° -> photons=" << totalPhotons 
                   << ", aluminum=" << totalDetected 
                   << G4endl;
        }

        fout.close();
        G4cout << "📁 Data saved to " << filename << G4endl;

        runManager->ReinitializeGeometry(true);
    }

    if (ui) {
        auto* UImanager = G4UImanager::GetUIpointer();
        UImanager->ApplyCommand("/control/macroPath ./macros");
        UImanager->ApplyCommand("/control/execute vis.mac");
        ui->SessionStart();
        delete ui;
    }

    delete visManager;
    delete runManager;
    return 0;
}
