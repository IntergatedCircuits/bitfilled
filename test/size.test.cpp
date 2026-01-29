#include "bitfilled/size.hpp"
#include <boost/ut.hpp>

using namespace bitfilled;
using namespace boost::ut;

const suite size = []
{
    "byte_width"_test = []
    {
        expect(byte_width(255u) == 1_u);
        expect(byte_width(256u) == 2_u);
        expect(byte_width(0) == 1_u);
        expect(byte_width(-128) == 1_u);
        expect(byte_width(-129) == 2_u);
    };
};
