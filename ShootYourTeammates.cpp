/*
Copyright (C) 2016, kongr45gpen
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "bzfsAPI.h"
#include "plugin_utils.h"

#include <math.h>

class ShootYourTeammates : public bz_Plugin {
public:
    virtual const char* Name () {return "Shoot Your Teammates";}
    virtual void Init (const char* config);
    virtual void Event (bz_EventData *eventData);
    virtual void Cleanup (void);
private:
    float floatingWins[256];
    float floatingLosses[256];

    // Includes the default ±1
    const float teamPenalty = 1.5;
    const float teamAward = 2.4;
};

BZ_PLUGIN(ShootYourTeammates)

void ShootYourTeammates::Init (const char* /*commandLine*/) {
    // Register our events with Register()
    Register(bz_ePlayerDieEvent);
    Register(bz_ePlayerJoinEvent);
}

void ShootYourTeammates::Cleanup (void) {
    Flush(); // Clean up all the events
}

void ShootYourTeammates::Event (bz_EventData *eventData) {
    switch (eventData->eventType) {
        case bz_ePlayerDieEvent: { // This event is triggered when a player sends the /kill Slash Command to kill another player
            bz_PlayerDieEventData_V1* dieData = (bz_PlayerDieEventData_V1*)eventData;

            // Data
            // ---
            //   (int)                   playerID       - ID of the player who was player.
            //   (bz_eTeamType)          team           - The team the player player was on.
            //   (int)                   killerID       - The owner of the shot that player the player, or BZ_SERVER for server side kills
            //   (bz_eTeamType)          killerTeam     - The team the owner of the shot was on.
            //   (bz_ApiString)          flagplayerWith - The flag name the owner of the shot had when the shot was fired.
            //   (int)                   shotID         - The shot ID that player the player, if the player was not player by a shot, the id will be -1.
            //   (bz_PlayerUpdateState)  state          - The state record for the player player at the time of the event
            //   (double)                eventTime      - Time of the event on the server.

            if (dieData->playerID == dieData->killerID) {
                // Don't handle self-kills
                return;
            }

            if (dieData->team == dieData->killerTeam && dieData->team != eRogueTeam && dieData->team != eNoTeam) {
                floatingWins[dieData->killerID] += teamAward - 1;
                floatingLosses[dieData->playerID] += teamPenalty - 1;

                // Update player scores according to floatingScore
                double addedScore;

                floatingWins[dieData->killerID] = modf(floatingWins[dieData->killerID], &addedScore);
                bz_incrementPlayerWins(dieData->killerID, addedScore);

                floatingLosses[dieData->playerID] = modf(floatingLosses[dieData->playerID], &addedScore);
                bz_incrementPlayerLosses(dieData->playerID, addedScore);
            }
        }
        break;

        case bz_ePlayerJoinEvent: { // This event is called each time a player joins the game
            bz_PlayerJoinPartEventData_V1* joinData = (bz_PlayerJoinPartEventData_V1*)eventData;

            // Data
            // ---
            //    (int)                   playerID  - The player ID that is joining
            //    (bz_BasePlayerRecord*)  record    - The player record for the joining player
            //    (double)                eventTime - Time of event.

            floatingWins[joinData->playerID] = floatingLosses[joinData->playerID] = 0;

            bz_sendTextMessagef(BZ_SERVER, joinData->playerID, "Being shot by a teammate costs you %g points", teamPenalty);
            bz_sendTextMessagef(BZ_SERVER, joinData->playerID, "Killing a teammate gives you %g points", teamAward);
        }
        break;

        default: break;
    }
}
