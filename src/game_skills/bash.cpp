#include "bash.h"
#include "fightsystem/pk.h"
#include "fightsystem/common.h"
#include "fightsystem/fight.h"
#include "fightsystem/fight_hit.h"
#include "protect.h"
#include "structs/global_objects.h"

// ************************* BASH PROCEDURES
void go_bash(CharData *ch, CharData *vict) {
	if (IsUnableToAct(ch) || AFF_FLAGGED(ch, EAffect::kStopLeft)) {
		send_to_char("Вы временно не в состоянии сражаться.\r\n", ch);
		return;
	}

	if (!(ch->is_npc() || GET_EQ(ch, kShield) || IS_IMMORTAL(ch) || GET_MOB_HOLD(vict)
		|| GET_GOD_FLAG(vict, EGf::kGodscurse))) {
		send_to_char("Вы не можете сделать этого без щита.\r\n", ch);
		return;
	};

	if (PRF_FLAGS(ch).get(EPrf::kIronWind)) {
		send_to_char("Вы не можете применять этот прием в таком состоянии!\r\n", ch);
		return;
	}

	if (ch->isHorsePrevents())
		return;

	if (ch == vict) {
		send_to_char("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}

	if (GET_POS(ch) < EPosition::kFight) {
		send_to_char("Вам стоит встать на ноги.\r\n", ch);
		return;
	}

	vict = TryToFindProtector(vict, ch);

	int percent = number(1, MUD::Skills()[ESkill::kBash].difficulty);
	int prob = CalcCurrentSkill(ch, ESkill::kBash, vict);

	if (GET_MOB_HOLD(vict) || GET_GOD_FLAG(vict, EGf::kGodscurse)) {
		prob = percent;
	}
	if (MOB_FLAGGED(vict, EMobFlag::kNoBash) || GET_GOD_FLAG(ch, EGf::kGodscurse)) {
		prob = 0;
	}
	bool success = percent <= prob;
	TrainSkill(ch, ESkill::kBash, success, vict);

	SendSkillBalanceMsg(ch, MUD::Skills()[ESkill::kBash].name, percent, prob, success);
	if (!success) {
		Damage dmg(SkillDmg(ESkill::kBash), fight::kZeroDmg, fight::kPhysDmg, nullptr);
		dmg.Process(ch, vict);
		GET_POS(ch) = EPosition::kSit;
		prob = 3;
	} else {
		//не дадим башить мобов в лаге которые спят, оглушены и прочее
		if (GET_POS(vict) <= EPosition::kStun && GET_WAIT(vict) > 0) {
			send_to_char("Ваша жертва и так слишком слаба, надо быть милосерднее.\r\n", ch);
			ch->setSkillCooldown(ESkill::kGlobalCooldown, kPulseViolence);
			return;
		}

		int dam = str_bonus(GET_REAL_STR(ch), STR_TO_DAM) + GetRealDamroll(ch) +
			MAX(0, ch->get_skill(ESkill::kBash) / 10 - 5) + GetRealLevel(ch) / 5;

//делаем блокирование баша
		if ((GET_AF_BATTLE(vict, kEafBlock)
			|| (IsAbleToUseFeat(vict, EFeat::kDefender)
				&& GET_EQ(vict, kShield)
				&& PRF_FLAGGED(vict, EPrf::kAwake)
				&& vict->get_skill(ESkill::kAwake)
				&& vict->get_skill(ESkill::kShieldBlock)
				&& GET_POS(vict) > EPosition::kSit))
			&& !AFF_FLAGGED(vict, EAffect::kStopFight)
			&& !AFF_FLAGGED(vict, EAffect::kMagicStopFight)
			&& !AFF_FLAGGED(vict, EAffect::kStopLeft)
			&& GET_WAIT(vict) <= 0
			&& GET_MOB_HOLD(vict) == 0) {
			if (!(GET_EQ(vict, kShield) || vict->is_npc() || IS_IMMORTAL(vict) || GET_GOD_FLAG(vict, EGf::kGodsLike)))
				send_to_char("У вас нечем отразить атаку противника.\r\n", vict);
			else {
				int range, prob2;
				range = number(1, MUD::Skills()[ESkill::kShieldBlock].difficulty);
				prob2 = CalcCurrentSkill(vict, ESkill::kShieldBlock, ch);
				bool success2 = prob2 >= range;
				TrainSkill(vict, ESkill::kShieldBlock, success2, ch);
				if (!success2) {
					act("Вы не смогли блокировать попытку $N1 сбить вас.",
						false, vict, nullptr, ch, kToChar);
					act("$N не смог$Q блокировать вашу попытку сбить $S.",
						false, ch, nullptr, vict, kToChar);
					act("$n не смог$q блокировать попытку $N1 сбить $s.",
						true, vict, nullptr, ch, kToNotVict | kToArenaListen);
				} else {
					act("Вы блокировали попытку $N1 сбить вас с ног.",
						false, vict, nullptr, ch, kToChar);
					act("Вы хотели сбить $N1, но он$G блокировал$G Вашу попытку.",
						false, ch, nullptr, vict, kToChar);
					act("$n блокировал$g попытку $N1 сбить $s.",
						true, vict, nullptr, ch, kToNotVict | kToArenaListen);
					alt_equip(vict, kShield, 30, 10);
					if (!ch->get_fighting()) {
						set_fighting(ch, vict);
						SetWait(ch, 1, true);
						//setSkillCooldownInFight(ch, ESkill::kBash, 1);
					}
					return;
				}
			}
		}

		prob = 0; // если башем убил - лага не будет
		Damage dmg(SkillDmg(ESkill::kBash), dam, fight::kPhysDmg, nullptr);
		dmg.flags.set(fight::kNoFleeDmg);
		dam = dmg.Process(ch, vict);

		if (dam > 0 || (dam == 0 && AFF_FLAGGED(vict, EAffect::kShield))) {
			prob = 2;
			if (!vict->drop_from_horse()) {
				GET_POS(vict) = EPosition::kSit;
				SetWait(vict, 3, false);
			}
		}
	}
	SetWait(ch, prob, true);
}

void do_bash(CharData *ch, char *argument, int/* cmd*/, int/* subcmd*/) {
	if ((ch->is_npc() && (!AFF_FLAGGED(ch, EAffect::kHelper))) || !ch->get_skill(ESkill::kBash)) {
		send_to_char("Вы не знаете как.\r\n", ch);
		return;
	}
	if (ch->haveCooldown(ESkill::kBash)) {
		send_to_char("Вам нужно набраться сил.\r\n", ch);
		return;
	};

	if (ch->ahorse()) {
		send_to_char("Верхом это сделать затруднительно.\r\n", ch);
		return;
	}

	CharData *vict = FindVictim(ch, argument);
	if (!vict) {
		send_to_char("Кого же вы так сильно желаете сбить?\r\n", ch);
		return;
	}

	if (vict == ch) {
		send_to_char("Ваш сокрушающий удар поверг вас наземь... Вы почувствовали себя глупо.\r\n", ch);
		return;
	}

	if (!may_kill_here(ch, vict, argument))
		return;
	if (!check_pkill(ch, vict, arg))
		return;

	if (IS_IMPL(ch) || !ch->get_fighting()) {
		go_bash(ch, vict);
	} else if (IsHaveNoExtraAttack(ch)) {
		if (!ch->is_npc())
			act("Хорошо. Вы попытаетесь сбить $N3.", false, ch, nullptr, vict, kToChar);
		ch->set_extra_attack(kExtraAttackBash, vict);
	}
}



// vim: ts=4 sw=4 tw=0 noet syntax=cpp :