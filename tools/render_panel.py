"""Render the firmware's actual HTML using a minimal host adapter; requires c++."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
src = (root / 'src/main.cpp').read_text()
adapter = r'''
#include <string>
#include <cstring>
#include <cstdint>
#include <iostream>
#define F(s) s
struct String : std::string {
  using std::string::string;
  String(size_t n) : std::string(std::to_string(n)) {}
};
void markActivity() {}
struct Server { void send(int, const char*, const String& s) { std::cout << s; } } server;
uint8_t activeRemote = 123, irVolumeRepeat = 2;
bool isFavorite(size_t i) { return i == 0 || i == 123; }
'''
parts = [adapter,
         src[src.index('enum IrProtocol'):src.index('BleKeyboard bleKeyboard')],
         src[src.index('const char *profileStatusLabel'):src.index('void redirectHome')],
         src[src.index('void handleRoot()'):src.index('void handleActivate()')],
         'int main() { handleRoot(); }']
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp) / 'panel.cpp'
    binary = Path(tmp) / 'panel'
    cpp.write_text('\n'.join(parts))
    subprocess.run(['c++', '-std=c++17', str(cpp), '-o', str(binary)], check=True)
    html = subprocess.check_output([str(binary)]).decode()
# Keep filters and speed preview; prevent every device action and upload in the public copy.
script = '''<script>
function demoNotice(){document.getElementById('demo-message').textContent='Anteprima: questa azione richiede il dispositivo Volumefy. Nessun comando è stato inviato.';}
document.addEventListener('submit',e=>{e.preventDefault();e.stopImmediatePropagation();demoNotice();},true);
document.addEventListener('click',e=>{const a=e.target.closest('a');if(a&&a.getAttribute('href').startsWith('/')){e.preventDefault();demoNotice();}},true);
</script>'''
banner = '''<aside style="max-width:880px;margin:16px auto;padding:16px;border:1px solid #64748b;border-radius:12px;background:#172033;color:#f3f4f6;font:14px/1.6 system-ui"><a href="index.html#pannello" style="color:#c8ee86">← Il progetto</a><strong style="display:block">Anteprima del pannello · dati dimostrativi</strong>Interfaccia generata dal firmware. Esplora tipo, marca, modello e velocità. Attivazione, preferiti, test e OTA funzionano solo sul dispositivo.<div id="demo-message" role="status" aria-live="polite"></div></aside>'''
html = html.replace('<body>', '<body>' + script + banner).replace('<title>Volumefy IR</title>', '<title>Volumefy IR — anteprima del pannello</title>')
(root / 'docs/panel.html').write_text(html)
print('Generated docs/panel.html from src/main.cpp')
