#include "PMSteppingAction.hh"
#include "PMEventAction.hh"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VProcess.hh"

PMSteppingAction::PMSteppingAction(PMEventAction* eventAction)
    : G4UserSteppingAction(),
      fEventAction(eventAction)
{}

PMSteppingAction::~PMSteppingAction() {}

void PMSteppingAction::UserSteppingAction(const G4Step* step)
{
    auto* track = step->GetTrack();

    // ----------------------------------------------------
    // OPTICAL PHOTONS
    // ----------------------------------------------------
    if (track->GetDefinition() == G4OpticalPhoton::OpticalPhotonDefinition()) {

        // First step: add to total optical photons + primary scintillation
        if (track->GetCurrentStepNumber() == 1 && fEventAction) {
            fEventAction->AddOpticalPhoton();

            const G4VProcess* creatorProcess = track->GetCreatorProcess();
            if (creatorProcess && creatorProcess->GetProcessName() == "Scintillation") {
                fEventAction->CountPrimaryPhoton();
            }
        }

        // Arrival at the aluminum detector surface:
        auto* postPoint = step->GetPostStepPoint();
        auto* postVol   = postPoint->GetPhysicalVolume();

        if (postVol && postVol->GetName() == "physAlDetector" && fEventAction) {

            const G4VProcess* creatorProcess = track->GetCreatorProcess();
            const G4String processName =
                creatorProcess ? creatorProcess->GetProcessName() : "";

            // Count scintillation photons only
            if (processName == "Scintillation") {
                G4double time_ns = postPoint->GetGlobalTime() / ns;

                // Increment counters
                fEventAction->AddAluminumPhoton();

                // Record arrival time: histogram + CSV + gamma/neutron averages
                fEventAction->RecordPhotonArrival(time_ns);
            }
        }

        return; // Skip the rest for optical photons
    }

    // ----------------------------------------------------
    // OTHER PARTICLES (gamma, e-, ...): energy deposition
    // ----------------------------------------------------
    G4double edep = step->GetTotalEnergyDeposit();
    if (edep > 0.0 && fEventAction) {
        fEventAction->RecordEnergy(edep);  // MeV
    }

    /*
    // Uncomment for debugging if needed
    if (edep > 0.1*MeV) {
        auto* preStep = step->GetPreStepPoint();
        G4String pname = track->GetDefinition()->GetParticleName();
        auto* vol = preStep->GetPhysicalVolume();
        G4String vname = vol ? vol->GetName() : "None";
        G4cout << "Energy Deposition: " << edep/MeV << " MeV by "
               << pname << " in " << vname << G4endl;
    }
    */
}
