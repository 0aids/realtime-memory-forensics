#include <chrono>
#include <string>
#include <thread>
using namespace std;
int __attribute__((optimize("O0"))) main() {

  char extrashit[] = "This is some extra shit";
  volatile char i[] = "ALL HAIL THE!!!";
  string *randomshit = new string("small string");

  string *otherrandomshit = new string(
      " Lorem ipsum dolor sit amet, consectetur adipiscing elit. Mauris "
      "commodo, felis non semper fermentum, lorem velit dictum risus, id "
      "rutrum ipsum nisi eget ex. Curabitur ullamcorper consectetur venenatis. "
      "Vestibulum ultrices iaculis arcu quis suscipit. Duis tincidunt "
      "venenatis fermentum. Fusce vehicula consequat erat, quis bibendum arcu "
      "iaculis a. In pharetra ligula massa. Pellentesque nisi ex, vehicula at "
      "est nec, molestie egestas enim. Integer eleifend eros in leo tempor, at "
      "gravida lectus hendrerit. Aenean at ligula eu leo pellentesque "
      "elementum. Morbi sed mattis metus, sit amet ultrices orci. Sed euismod, "
      "ex sed hendrerit sagittis, nibh sapien lobortis nisl, ut dignissim "
      "tellus nulla ut ex. Phasellus eu convallis odio. Morbi dignissim "
      "bibendum lorem, quis sollicitudin est vehicula facilisis. Cras "
      "venenatis in enim vitae efficitur. Sed lacinia viverra turpis elementum "
      "maximus. Nam sit amet rhoncus nisl. Etiam dapibus ligula vel faucibus "
      "dapibus. Curabitur commodo ante non arcu efficitur hendrerit. Ut "
      "consectetur scelerisque nulla, id molestie metus commodo ac. Etiam a "
      "efficitur orci, ac bibendum velit. In lacinia viverra ex id euismod. "
      "Sed nec dolor vel purus rutrum pharetra ac non est. Nullam lobortis "
      "rutrum ligula, nec sagittis sem bibendum nec. Vivamus sagittis ligula "
      "enim, et tempus ex porttitor in. Cras mattis ex in nibh tempus, non "
      "rhoncus risus iaculis. Sed tincidunt elementum ligula in convallis. Nam "
      "faucibus ipsum in sagittis dignissim. Nam a elit mauris. Pellentesque "
      "auctor erat sed pellentesque tempor. Etiam at congue ante. "
      "Pellentesque ");
  while (true) {
    this_thread::sleep_for(chrono::seconds(5));
    i[0]++;
    i[0] = (i[0] - 65) % 26 + 65;
  }
  delete randomshit;
  delete otherrandomshit;
}
