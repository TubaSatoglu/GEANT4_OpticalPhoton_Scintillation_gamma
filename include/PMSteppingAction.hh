#ifndef PMSTEPPINGACTION_HH
#define PMSTEPPINGACTION_HH

#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4ParticleDefinition.hh"
#include "G4OpticalPhoton.hh"
#include "G4SystemOfUnits.hh"
#include "G4String.hh"

class G4Step;
class PMEventAction;

class PMSteppingAction : public G4UserSteppingAction {
public:
  explicit PMSteppingAction(PMEventAction* eventAction);
  ~PMSteppingAction() override;

  void UserSteppingAction(const G4Step* step) override;

private:
  bool IsRelevantParticle(const G4String& particleName) const;
  PMEventAction* fEventAction;
};

#endif // PMSTEPPINGACTION_HH
