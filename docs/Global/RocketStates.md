# ROCKET STATES

IDLE

FUEL
 - ... (see fsm) // TODO

ABORT -> fuelling and flight aborted
READY -> fuel ok, system ok, can arm
ARMED -> ready for launch (can trigger e-matches)

FLIGHT
 - LAUNCH -> e-maches triggered
 - ASCEND -> in flight
 - APOGEE -> apogee detected
 - DROGUE_CHUTE -> drogue chute open
 - MAIN_CHUTE -> main chute open
 - TOUCHDOWN -> landed
