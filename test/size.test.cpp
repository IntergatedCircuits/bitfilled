#include "bitfilled/size.hpp"
#include <boost/ut.hpp>

using namespace bitfilled;
using namespace boost::ut;

const suite<"size"> size = []
{
    "byte_width"_test = []
    {
        expect(byte_width(255U) == 1_u);
        expect(byte_width(256U) == 2_u);
        expect(byte_width(0) == 1_u);
        expect(byte_width(-128) == 1_u);
        expect(byte_width(-129) == 2_u);
    };
};
