#include "error.h"
#include "state.h"
#include <assert.h>

void
notimplemented() {
  std::cout << "Instruction at PC: 0x" << std::hex << s->data.r[0] << std::endl
            << s->instructionForAddr(s->data.r[0])->toString() << " not implemented" << std::endl;
  s->data.running = false;
}
