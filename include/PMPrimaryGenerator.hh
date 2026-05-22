#ifndef PMPRIMARYGENERATOR_HH
#define PMPRIMARYGENERATOR_HH

#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ThreeVector.hh"
#include "G4SystemOfUnits.hh"

class G4Event;

class PMPrimaryGenerator : public G4VUserPrimaryGeneratorAction {
public:
  explicit PMPrimaryGenerator(G4double energy);
  ~PMPrimaryGenerator() override;

  void GeneratePrimaries(G4Event* anEvent) override;

  void SetGammaEnergy(G4double energy)   { fBaseEnergy = energy;     }
  G4double GetGammaEnergy() const        { return fBaseEnergy;       }
  void SetEnergySigma(G4double sigma)    { fEnergySigma = sigma;     }
  G4double GetEnergySigma() const        { return fEnergySigma;      }
  void SetSourceDistance(G4double dist)  { fSourcePosZ = dist;       }
  G4double GetSourceDistance() const     { return fSourcePosZ;       }
  void SetSourceRadius(G4double radius)  { fSourceRadius = radius;   }
  G4double GetSourceRadius() const       { return fSourceRadius;     }
  void SetBeamTheta(G4double theta)      { fBeamTheta = theta;       }
  G4double GetBeamTheta() const          { return fBeamTheta;        }

  void SetIncidenceAngle(G4double theta) { SetBeamTheta(theta);      }

private:
  void UpdatePositionAndDirection();

  G4ParticleGun* fParticleGun;
  G4double fBaseEnergy;   // center energy
  G4double fEnergySigma;  // Gaussian sigma
  G4double fSourcePosZ;   // source z-position
  G4double fSourceRadius; // (unused)
  G4double fBeamTheta;    // beam angle
};

#endif
