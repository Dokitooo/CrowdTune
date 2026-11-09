CrowdTune 1.0 - a behaved, seated regular-season crowd for NBA 2K14 (PC, RLD nba2k14.exe)
=========================================================================================
Stock 2K14 fans behave like a soccer end: half the arena stands, claps or waves props during normal
possessions. CrowdTune keeps them seated and quiet during normal play, gives routine baskets mild
seated applause, and saves the big reactions for the moments that earn them.

NEEDS: SLAM 1.0 (the SLAM folder + dinput8.dll next to nba2k14.exe). Works with the stock crowd.iff
(a texture-only crowd mod is fine too).

INSTALL
  Drag the folder SLAM\CrowdTune from this zip into the game's SLAM folder:
      SLAM\CrowdTune\CrowdTune.asi
      SLAM\CrowdTune\CrowdTune.ini
  The mod list at the top left shows "CrowdTune 1.0". Log: SLAM\CrowdTune\CrowdTune.log.
  Updating later: keep a copy of your CrowdTune.ini first (or skip the .ini when copying).
  Remove: delete SLAM\CrowdTune (or Enabled=0 in CrowdTune.ini).

WHAT IT DOES (all settings in CrowdTune.ini)
- Attendance: the arena isn't sold out every night. The game fills seats from the home team's
  popularity; CrowdTune shows Attendance (85%) of that in the regular season, PlayoffAttendance (100%)
  in the playoffs. Empty seats are spread around the bowl.
- Everybody is seated when the arena loads. Normal play: seated, quiet idle; a seated fan stays seated
  for at least CalmHoldSeconds (12). Intros, tip-off and breaks are calm too.
- Props (towels, thunder sticks, foam fingers, signs) and chants: playoff games only.
- The crowd follows the game (score, quarter, game clock, shot clock):
    home basket -> calm seated claps, a three a bit more
    home dunk / and-one -> amazed: standing, hands up, claps
    away dunk / and-one (half the arena) -> disbelief, hands on / behind heads; the rest sits quietly
    shot released before the quarter / shot-clock horn that goes in after it -> WOW (bigger for a
      three); the opponent's -> disbelief. A horn on its own gets no reaction.
    close finish (4th / OT, within 5, last 2:00 scaled to your quarter length) -> up to 80% standing;
      tied at the horn they stay up through the break before overtime
    home buzzer beater to win or tie -> the biggest reaction
    final horn -> win / loss reaction right at the horn for 40 s (close win ~90% up, loss: upset)
- 3rd quarter: fans head out at halftime; 55% are back at the restart, full by mid-quarter.
- 4th quarter: the opponent cutting into a home lead, tying or taking it -> upset, more the bigger
  the run. Fans leave only while the opponent leads by 15+.
- Reactions rotate around the arena: each event uses a different set of the game's 60 animation
  channels, so it isn't the same people jumping up every time.

HOW IT WORKS
The arena runs 60 shared "fan channels"; each seat follows one. When a channel's clip ends, the game
picks its next mood (idle / applause / cheer / disappointed / props / chant). CrowdTune re-maps that
choice from the game situation and the scoreboard. Stand-ups are naturally staggered: each channel
only switches when its current loop ends, so the arena rises in a ripple. Home team = the arena's
team (HomeTeam= overrides).

LIMITS
- Lower and upper bowl can't be treated differently (seats share the 60 channels at random).
- Every seat on one channel moves together - that is the game's design.
- If a crowd mod changes the clip graph in crowd.iff, CrowdTune leaves it alone.
- Only the RLD exe (TimeDateStamp 52202C7F); anything else is left untouched.

TUNING WITH THE LOG
CrowdTune.log writes a line every 2 s while the crowd is active: excitement, situation code, phase,
asked/given per mood group and what the 60 channels play.
  Mood groups (men|women): 1 sit idle, 6 stand idle, 2 applause, 3 sit cheer, 7 stand cheer,
  4 sit sad, 5 stand sad. Both: 8 towel, 9 thunder sticks, 10 foam finger, 11 sign, 15-17 chants.

FILES
SLAM\CrowdTune\CrowdTune.asi + CrowdTune.ini
