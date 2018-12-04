#include "parabot.h"
#include "pb_goalfinder.h"
//#include "pb_goals.h"

PB_GoalFinder::~PB_GoalFinder()
{
	knownGoals.clear();
}


void PB_GoalFinder::init( CParabot *pb )
// initializes all necessary variables
{
	bot = pb;
#if DEBUG
	bot->goalMove[0] = 0;
	bot->goalView[0] = 0;
	bot->goalAct[0] = 0;
#endif
	memset( &bestGoalFunction, 0, sizeof bestGoalFunction );
	memset( &responsiblePercept, 0, sizeof responsiblePercept );
	memset( &bestWeight, 0, sizeof bestWeight );
}


void PB_GoalFinder::addGoal( int goalClass, int triggerId, tGoalFunc gf, tWeightFunc wf )
{
	tGoal g;
	g.type = goalClass;
	g.goal = gf;
	g.weight = wf;
	knownGoals.insert( std::make_pair(triggerId, g) );
}


void PB_GoalFinder::analyze( PB_Perception &senses )
{
	float weight;
	int   type;
	tPerceptionList *perception = senses.perceptionList();
	tPerceptionList::iterator pli = perception->begin();

	while ( pli != perception->end() ) {
		/*PB_Percept dmgp = *pli;
		if ( pli->pClass == PI_DAMAGE ) {
			DEBUG_MSG( "damage sensed\n" );
		}*/
		if ( worldtime() >= pli->update ) {
			// rating only necessary for enemies:
			if (pli->pClass == PI_FOE) {
				assert (pli->entity != 0);
				// check if enemy can see the bot
				makevectors(&pli->entity->v.v_angle);
				Vec3D dir;
				vsub(bot->botPos(), &pli->entity->v.origin, &dir);
				normalize(&dir);
				pli->orientation = dotproduct(&com.globals->fwd, &dir);
				// weapon rating ( -5 .. +5 )
				if (pli->hasBeenSpotted())
					pli->rating = combat_getrating(&bot->combat, (*pli)); 
				else
					pli->rating = 0;
				
				// DEBUG_MSG( "Rating %s = %f\n", STRING(pli->entity->v.netname), pli->rating );
				//float dst = (bot->botPos() - pli->entity->v.origin).Length();
				//float hp = bot->action.targetAccuracy();
				// DEBUG_MSG("HitProb = %.2f\n", hp );
			}	
			tGoalList::iterator gli = knownGoals.find(pli->pClass);
			while (gli != knownGoals.end()) {
				// DEBUG_MSG( "." );
				assert(gli->first == pli->pClass);
				type = gli->second.type;
				tWeightFunc wf = gli->second.weight;
				if (wf)
					weight = (*wf)( bot, &(*pli) );
				else
					weight = -100;

				if (weight > bestWeight[type]) {
					bestGoalFunction[type] = gli->second.goal;
					responsiblePercept[type] = &(*pli);
					assert(responsiblePercept[type]->pClass > 0 && responsiblePercept[type]->pClass <= MAX_PERCEPTION);
					bestWeight[type] = weight;
				}
				// next goal for this perception:
				do{
					gli++;
				} while ((gli != knownGoals.end()) && (gli->first != pli->pClass));
			}
			// DEBUG_MSG( "\n" );
		}
		pli++;
		//check();
	}
	//check();
}

void PB_GoalFinder::analyzeUnconditionalGoals()
{
	float weight;
	int   type;
	tGoalList::iterator gli = knownGoals.begin();

	while ( gli != knownGoals.end() ) {
		if (gli->first == GOAL_UNCONDITIONAL) {
			type = gli->second.type;
			tWeightFunc wf = gli->second.weight;
			if (wf)
				weight = (*wf)( bot, 0 );
			else
				weight = -100;

			if (weight > bestWeight[type]) {
				bestGoalFunction[type] = gli->second.goal;
				responsiblePercept[type] = 0;
				bestWeight[type] = weight;
			}
		}
		gli++;
	}
}


void PB_GoalFinder::synchronize()
// deletes goals that can't be executed at the same time
{
	if (bestWeight[G_MOVE|G_VIEW] > (bestWeight[G_MOVE]+bestWeight[G_VIEW])) {
		bestGoalFunction[G_MOVE] = 0;
		responsiblePercept[G_MOVE] = 0;
		bestGoalFunction[G_VIEW] = 0;
		responsiblePercept[G_VIEW] = 0;
	} else {
		bestGoalFunction[G_MOVE|G_VIEW] = 0;
		responsiblePercept[G_MOVE|G_VIEW] = 0;
	}
}


void PB_GoalFinder::check()
{
	for (int i = 0; i < MAX_GOALS; i++) {
		if (responsiblePercept[i] != 0) {
			assert( responsiblePercept[i]->pClass > 0 && responsiblePercept[i]->pClass <= MAX_PERCEPTION );
		}
	}
}
