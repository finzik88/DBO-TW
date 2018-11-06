#include <Game\Object\Manager\Fight.h>
#include <TableAll.h>
#include <Game\Object\Player.h>
#include <Game\Object\Mob.h>

//----------------------------------------
//	Constructor
//----------------------------------------
FightManager::FightManager()
{

}
//----------------------------------------
//	Destructor
//----------------------------------------
FightManager::~FightManager()
{

}
//----------------------------------------
//	Can we damage ou target ?
//	Need to check if the flag is set to attackable etc
//----------------------------------------
bool FightManager::canTakeDamage(Object& Attacker, Object& Target)
{
	if (Target.GetTypeId() == eOBJTYPE::OBJTYPE_MOB || Target.GetTypeId() == OBJTYPE_PC)
	{
		
	}
	return false;
}
//----------------------------------------
//	Get the amount of level between us and mob
//----------------------------------------
int FightManager::GetLevelDiff()
{
	int levelDiff = plr->GetPcProfile()->byLevel - mob->GetMobData().Level;

	return levelDiff;
}
//----------------------------------------
//	Get amount of damage to do
//	@param id - boolean to say if we are caster or cac
//----------------------------------------
void FightManager::GetPlayerDamage(bool caster)
{
	attackValue = 0;
	if (caster == false)
	{
		attackValue = plr->GetPcProfile()->avatarAttribute.wLastPhysicalOffence;
		attackValue -= mob->GetMobData().Basic_physical_defence/3;
	}
	else
	{
		attackValue = plr->GetPcProfile()->avatarAttribute.wLastEnergyOffence;
		attackValue -= mob->GetMobData().Basic_energy_defence/3;
	}
}
//----------------------------------------
//	Get if our attack will be critical
//	@param id - boolean to say if we are caster or cac
//----------------------------------------
void FightManager::GetPlayerCriticAttack(bool caster)
{
	int CritChance = 0; 
	int num = rand() % 100 + 1;

	if (caster == false)
	{
		CritChance = plr->GetPcProfile()->avatarAttribute.wLastPhysicalCriticalRate - (mob->GetMobData().Block_rate / 100);
	}
	else
	{
		CritChance = plr->GetPcProfile()->avatarAttribute.wLastEnergyCriticalRate - (mob->GetMobData().Block_rate / 100);
	}
	(CritChance > 0) ? CritChance *= 1 : CritChance *= -1;
	if (num <= CritChance && CritChance > 0)
	{
		attackResult = eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_CRITICAL_HIT;
		attackValue *= 2;
	}
	//std::cout << "CritChance = " << CritChance << std::endl;
}
//----------------------------------------
//	Get the player hit change
//----------------------------------------
void FightManager::GetPlayerHitChance()
{
	float percent = ((mob->GetMobData().Dodge_rate - plr->GetPcProfile()->avatarAttribute.wLastAttackRate) / ((0.3605f * mob->GetMobData().Level) + 18.64f) * ((0.335f * plr->GetPcProfile()->byLevel) + 50)) * -1;

	int num = rand() % 100 + 1;
	if (num <= percent && percent > 0)
		attackResult = eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_HIT;
	//std::cout << "Hitchance = " << percent << std::endl;
	//std::cout << "Target dodge: " << mob->GetMobData().Dodge_rate << std::endl;
	//if (num <= mob->GetMobData().Block_rate)
		//attackResult = eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_BLOCK;
}
//----------------------------------------
//	Perform an auto attack from an object
//	@param id - Attacker - Target as Object
//----------------------------------------
bool FightManager::HandleDamage(Object& Attacker, Object& Target)
{
	mutexFight.lock();
	if (Target.GetIsDead() == true)
	{
		mutexFight.unlock();
		return false;
	}
	if (Attacker.GetTypeId() == OBJTYPE_PC)
	{
		plr = reinterpret_cast<Player*>(&Attacker);
		mob = reinterpret_cast<Mob*>(&Target);
		float distance = NtlGetDistance(plr->GetVectorPosition().x, plr->GetVectorPosition().z, mob->GetMobData().curPos.x, mob->GetMobData().curPos.z);
		if (distance <= (plr->GetPcProfile()->avatarAttribute.fLastAttackRange) + 2)
		{
			/// can now continue attack verification for player
			HandlePlrFight();
			attackCount++;
		}
	}
	else
	{
		if (NtlGetDistance(plr->GetVectorPosition().x, plr->GetVectorPosition().z, mob->GetMobData().curPos.x, mob->GetMobData().curPos.z) <= mob->GetMobData().Attack_range)
		{
			/// can now continue attack verification for mob
		}
	}
	mutexFight.unlock();
	return true;
}
//----------------------------------------
//	Handle an attack from a player
//----------------------------------------
void FightManager::HandlePlrFight()
{
	if (attackCount > BATTLE_MAX_CHAIN_ATTACK_COUNT || attackCount <= 0)
		attackCount = BATTLE_CHAIN_ATTACK_START;
	//Type Atack
	bool isCaster = false;
	if (plr->GetMyClass() == ePC_CLASS::PC_CLASS_HUMAN_FIGHTER || 
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_NAMEK_FIGHTER || 
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_MIGHTY_MAJIN ||
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_STREET_FIGHTER ||
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_SWORD_MASTER ||
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_DARK_WARRIOR ||
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_SHADOW_KNIGHT ||
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_ULTI_MA ||
		plr->GetMyClass() == ePC_CLASS::PC_CLASS_GRAND_MA)
		isCaster = false;
	else
		isCaster = true;

	sGU_CHAR_ACTION_ATTACK res;

	attackResult = eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_DODGE;
	guardType = eDBO_GUARD_TYPE::DBO_GUARD_TYPE_INVALID;
	attackValue = 0;
	reflectedDamage = 0;
	
	GetPlayerDamage(isCaster);
	GetPlayerHitChance();
	if (attackValue <= 0 || attackValue > 1000000000)
	{
		attackValue = 1;
	}
	if (attackResult != eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_DODGE)
	{
		GetPlayerCriticAttack(isCaster);
		if (attackResult == eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_BLOCK)
		{
			attackValue /= 2;
		}
	}
	else
		attackValue = 0;
	
	res.bChainAttack = true;
	res.byAttackResult = attackResult;
	res.byBlockedAction = guardType;
	res.dwLpEpEventId = 0;
	res.fReflectedDamage = reflectedDamage;
	res.byAttackSequence = attackCount;
	res.hSubject = plr->GetHandle();
	res.hTarget = mob->GetHandle();
	res.vShift = plr->GetVectorPosition();
	res.wAttackResultValue = attackValue;
//	for (int i = 0; i < 9; i++)res.unknown[i] = 0;

	res.wOpCode = GU_CHAR_ACTION_ATTACK;
	res.wPacketSize = sizeof(sGU_CHAR_ACTION_ATTACK) - 2;
	
	plr->SendPacket((char*)&res, sizeof(sGU_CHAR_ACTION_ATTACK));
	plr->SendToPlayerList((char*)&res, sizeof(sGU_CHAR_ACTION_ATTACK));

	if (attackResult != eBATTLE_ATTACK_RESULT::BATTLE_ATTACK_RESULT_DODGE)
	{
		if (mob->attackers == 0)
			mob->attackers = plr->GetHandle();
		mob->TakeDamage(attackValue);
	}
}