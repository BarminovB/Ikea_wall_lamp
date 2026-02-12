"""Patch webgui.cpp: remove old City Clock Settings, add city dropdown."""
import gzip
import re
import sys

WEBGUI_PATH = "src/webgui.cpp"

# New script: dropdown with 4 cities, auto-shows when City Clock plugin selected
INJECT_SCRIPT = r"""<script>!function(){var w,id,C=['Helsinki','Omsk','Berlin','St. Petersburg'];function cn(){w=new WebSocket('ws://'+location.host+'/ws');w.onmessage=function(e){try{var d=JSON.parse(e.data);if(d.plugins)for(var i=0;i<d.plugins.length;i++)if(d.plugins[i].name==='City Clock'){id=d.plugins[i].id;break}}catch(x){}};w.onclose=function(){setTimeout(cn,3e3)}}cn();setInterval(function(){var s=document.querySelectorAll('select');if(!s.length)return;var p=s[0],t=p.options[p.selectedIndex],ok=t&&t.textContent==='City Clock',d=document.getElementById('ccs');if(!ok){if(d)d.remove();return}if(d)return;d=document.createElement('div');d.id='ccs';d.innerHTML='<div class="my-6 border-t border-gray-200"></div><div class="space-y-3"><h3 class="text-sm font-semibold text-gray-700 uppercase tracking-wide">City</h3><div class="space-y-2"><select id="ccSel" class="flex-1 px-2.5 py-2.5 bg-gray-50 border border-gray-200 rounded w-full"></select></div></div>';var e=d.querySelector('#ccSel');for(var i=0;i<C.length;i++){var o=document.createElement('option');o.value=i;o.textContent=C[i];e.appendChild(o)}var divs=document.querySelectorAll('[class*="border-t"][class*="border-gray"]');for(var i=0;i<divs.length;i++){var nx=divs[i].nextElementSibling;if(nx){var h3=nx.querySelector('h3');if(h3&&h3.textContent.indexOf('Brightness')>=0){divs[i].parentElement.insertBefore(d,divs[i]);break}}}if(!d.parentElement){var btns=document.querySelectorAll('button');for(var i=0;i<btns.length;i++){if(btns[i].textContent.indexOf('Default')>=0){btns[i].closest('.space-y-3,.flex-col').after(d);break}}}fetch('/api/cityclock').then(function(r){return r.json()}).then(function(j){if(j.cityIndex!==undefined)e.value=j.cityIndex}).catch(function(){});e.onchange=function(){if(w&&w.readyState===1&&id)w.send(JSON.stringify({event:'cityclock',cityIndex:+this.value,plugin:id}))}},500)}()</script>"""

def main():
    with open(WEBGUI_PATH, "r", encoding="utf-8") as f:
        content = f.read()

    match = re.search(r'GUI_HTML\[\] PROGMEM = \{([^}]+)\}', content)
    if not match:
        print("ERROR: Could not find GUI_HTML array")
        sys.exit(1)

    byte_values = [int(b.strip()) for b in match.group(1).split(',') if b.strip()]
    print(f"Extracted {len(byte_values)} bytes")

    compressed = bytes(byte_values)
    html = gzip.decompress(compressed).decode('utf-8')
    print(f"Decompressed: {len(html)} chars")

    # Remove old City Clock Settings script block
    # Find the script containing 'cc-city' or 'cc-save' or 'City Clock Settings'
    old_pattern = re.compile(r'<script>\(function\(\)\{\s*var ccId.*?\}\)\(\)</script>', re.DOTALL)
    old_match = old_pattern.search(html)
    if old_match:
        print(f"Removing old City Clock script ({old_match.end()-old_match.start()} chars)")
        html = html[:old_match.start()] + html[old_match.end():]
    else:
        print("No old City Clock script found")

    # Remove any existing new injection too (for re-patching)
    html = re.sub(r'<script>!function\(\)\{var w,id,C=.*?\}\(\)</script>', '', html, flags=re.DOTALL)

    # Inject new script before </body>
    if '</body>' in html:
        html = html.replace('</body>', INJECT_SCRIPT + '</body>', 1)
    else:
        html += INJECT_SCRIPT

    print(f"Patched: {len(html)} chars")

    new_compressed = gzip.compress(html.encode('utf-8'), compresslevel=9)
    print(f"Recompressed: {len(new_compressed)} bytes (was {len(compressed)})")

    lines = []
    for i in range(0, len(new_compressed), 30):
        chunk = new_compressed[i:i+30]
        lines.append(','.join(str(b) for b in chunk))
    array_content = ',\n'.join(lines)

    new_content = f'''#include "webgui.h"

#ifdef ENABLE_SERVER

#include <ESPAsyncWebServer.h>

const uint32_t GUI_HTML_SIZE = {len(new_compressed)};
const uint8_t GUI_HTML[] PROGMEM = {{{array_content}}};

void startGui(AsyncWebServerRequest *request)
{{
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", GUI_HTML, GUI_HTML_SIZE);
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}}

#endif
'''

    with open(WEBGUI_PATH, "w", encoding="utf-8") as f:
        f.write(new_content)

    print(f"Done! Updated {WEBGUI_PATH}")

if __name__ == "__main__":
    main()
