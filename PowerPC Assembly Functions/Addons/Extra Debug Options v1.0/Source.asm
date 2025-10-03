##############################################################
[CM_Addons] gfAreaManager display area id [Eon, ilikepizza107]
##############################################################
.include "Source/CM_Addons/AddonAliases.asm"

.macro lwi(<reg>, <val>)
{
    .alias  temp_Hi = <val> / 0x10000
    .alias  temp_Lo = <val> & 0xFFFF
    lis     <reg>, temp_Hi
    ori     <reg>, <reg>, temp_Lo
}

#getAreaColour
HOOK @ $80541FD0
{
	stwu r1, -0x10(r1)
	mflr r0
	stw r0, 0x14(r1)
	stw r31, 0x8(r1)
	mr r31, r3
	lis r3, XTRADBUG_VALUE_LOC_HI		# \
	lwz r3, XTRADBUG_VALUE_LOC_LO(r3)  	# / Get address of line
	lwz r3, 0x08(r3)                   	# grab currently selected ID

	mr r5, r3
	lwz r4, 0x8(r31)
	li r6, 1
	slw r6, r6, r5
	and. r0, r4, r6
	bne listener
	lbz r6, 0x1D(r31)
	cmpw r5, r6
	beq presenter
dontDraw:
	li r3, 0
	b end
listener:
	lis r3, 0x00AA
	ori r3, r3, 0xAAFF
	b end
presenter:
	lis r3, 0x0 
	ori r3, r3, 0xFFFF
end:
	lwz r31, 0x8(r1)
	lwz r0, 0x14(r1)
	mtlr r0
	addi r1, r1, 0x10
	blr
}

#debug check
HOOK @ $80012908
{
	li r3, 1
	cmpwi r3, 0
	mr r3, r31
}

####
#rectangles
####
#for "off" boxes
#set "off" colour to transparent, catch this is renderer
word 0x00000000 @ $8059ff8c
#gfAreaRect
CODE @ $8001146C
{
	stw r3, 0x8(r1)
	lis r12, 0x8054
	ori r12, r12, 0x1FD0
	mtctr r12
	bctrl
	stw r3, 0xC(r1)
	lwz r3, 0x8(r1)
	addi r3, r3, 0x2C
	addi r4, r1, 0xC
	li r5, 1
	lfs f1, -0x7F80(r2)
	nop 
}
#gfAreaCircle
CODE @ $80011798
{
	lis r12, 0x8054
	ori r12, r12, 0x1FD0
	mtctr r12
	bctrl
	stw r3, 0xC(r1)

	addi r3, r1, 0x10
	addi r4, r1, 0xC
	li r5, 1
	lfs f1, -0x7F80(r2)
	nop 
	nop 
	nop 
}
#gfAreaTriangle
CODE @ $80011BC8
{
	lis r12, 0x8054
	ori r12, r12, 0x1FD0
	mtctr r12
	bctrl
	stw r3, 0xC(r1)

	addi r3, r1, 0x10
	addi r4, r1, 0xC
	li r5, 1
	lfs f1, -0x7F80(r2)
	nop 
	nop 
	nop 
}



/*
0 = nop?, empty presenter used by boxes that dont outwardly state their function
1 = surrounds ecb, unknown purpose
2 = footstool event
3 = under feat in air
4 = item grabbox presented action? if an item wants to know if in range? no items have the counterpart afaik
5 = eatbox (17) presented action, same as above
6 =
7 =
8 = 
9 = surrounds ecb of all subspace enemies
10 = unk 
11 = subspace ai controller logic? theres a tonne of presenters on each primid + surrounding entire stage
12 =  
13 = 
14 = footstoolbox for subspace enemies e.g. koopas/goombas
15 = 
16 = 
17 = kirby/wario/dedede item "eatbox", also used for shell item bounce e.g. autofootstol. also on all items even if not edible/jumpable
18 = small item
19 = large item
20 = unk 
21 = doors/subspace springs/Catapult/barrel
22 = ladder
23 = elevator
24 = unk,
25 = surrounds ecb 
26 = whispy wind, gravity changer etc + aesthetic wind.
27 = conveyor
28 = water 
29 = Dedede Waddle grab range 
30 = surrounds ecb
31 = spring

#0x8  target group
#0xC  target local group
#0x14 team id
*/