#include "../python_globals.h"
#include "../python_property.h"

#include <plask/properties/electrical.h>

namespace plask { namespace python {

void register_standard_properties_voltage()
{
    registerProperty<Voltage>();
}

}} // namespace plask::python
