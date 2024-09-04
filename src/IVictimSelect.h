#pragma once
#include <memory>

#include "Macro.h"

//공격 대상 관련 처리.
//


namespace psh
{
    enum class ServerType : unsigned char;
}

namespace psh {
    class Field;
    struct AttackInfo;

    class IVictimSelect {
        IS_INTERFACE(IVictimSelect)

        virtual void GetVictim(const AttackInfo &attackInfo) const = 0;
    };

    class PveVictim final : public IVictimSelect {
    public:
        explicit PveVictim( Field * const field)
            : IVictimSelect()
              , _field(field) {
        }

        void GetVictim(const AttackInfo &attackInfo) const override;

    private:
         Field * const _field;
    };

    class PvpVictim final : public IVictimSelect {
    public:
        explicit PvpVictim(Field * const field)
            : IVictimSelect()
              , _field(field) {
        }

        void GetVictim(const AttackInfo &attackInfo) const override;

    private:
        Field * const _field;
    };


    namespace victim_select
    {
        std::unique_ptr<IVictimSelect> GetVictimByServerType(ServerType type, Field& field);
    }

}
