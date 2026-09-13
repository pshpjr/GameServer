//
// Created by pshpj on 24. 10. 9.
//

#ifndef UPDATABLE_H
#define UPDATABLE_H

namespace psh
{
    class Updatable {
    public:
        virtual ~Updatable() = default;

        virtual void Update(int delta) = 0;
    };
}

#endif //UPDATUBLE_H
