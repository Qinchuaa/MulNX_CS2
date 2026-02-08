#pragma once

#include"Pawn/Pawn.hpp"
#include"Controller/Controller.hpp"

class C_Entity {
public:
	int IndexInEntityList{ -1 };

    std::ostringstream GetMsg()const;

    C_PlayerController Controller{};
    C_Pawn Pawn{};

    C_Pawn& GetPawn() {
        return this->Pawn;
    }
    C_PlayerController& GetController() {
        return this->Controller;
    }
}; 