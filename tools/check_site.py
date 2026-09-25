"""Optional browser check: pip install playwright; requires installed Google Chrome."""
import tempfile
from pathlib import Path
from playwright.sync_api import sync_playwright
root=Path(__file__).resolve().parents[1]
with sync_playwright() as p:
 browser=p.chromium.launch(channel="chrome")
 page=browser.new_page(viewport={'width':940,'height':1060},device_scale_factor=1)
 errors=[]
 page.on('pageerror',lambda e:errors.append(str(e)))
 page.goto((root/'docs/panel.html').as_uri())
 # Render the same firmware content for illustrations, excluding the separate demo banner.
 page.locator('aside').evaluate('(e)=>e.style.display="none"')
 page.screenshot(path=str(root/'docs/images/panel-desktop.png'))
 speed=page.locator('.card').filter(has=page.get_by_role('heading',name='Velocità volume IR'))
 page.set_viewport_size({'width':940,'height':820})
 speed.scroll_into_view_if_needed()
 page.evaluate('(y)=>window.scrollTo(0,y-18)',speed.evaluate('(e)=>e.offsetTop'))
 page.screenshot(path=str(root/'docs/images/panel-ota.png'))
 page.reload()
 assert page.locator('#model').input_value()=='123'
 page.locator('#type').select_option('TV')
 page.locator('#brand').select_option('LG')
 page.locator('#search').fill('OLED')
 assert page.locator('#model option').count()==3
 page.locator('#search').fill('no-such-model')
 assert page.locator('#model option').count()==0
 assert 'Nessun modello' in page.locator('#modelInfo').inner_text()
 page.locator('#type').select_option('Soundbar')
 page.locator('#brand').select_option('Philips')
 page.locator('#search').fill('HTL')
 assert page.locator('#model option').count()==7
 page.locator('#speed').fill('6')
 assert page.locator('#speedValue').inner_text()=='6×'
 requests=[]
 page.on('request',lambda r:requests.append(r.url))
 page.get_by_role('button',name='Imposta come attivo').click()
 page.get_by_role('link',name='Volume +',exact=True).click()
 page.get_by_role('button',name='Salva velocità').click()
 page.locator('#otaFile').set_input_files({'name':'demo.bin','mimeType':'application/octet-stream','buffer':b'not-firmware'})
 page.get_by_role('button',name='Aggiorna firmware').click()
 assert not requests, requests
 assert 'Nessun comando' in page.locator('#demo-message').inner_text()
 for width in [1440,390,320]:
  page.set_viewport_size({'width':width,'height':1000})
  page.goto((root/'docs/index.html').as_uri())
  assert page.evaluate('document.documentElement.scrollWidth <= innerWidth'),f'overflow {width}'
  for img in page.locator('img').all():
   if not img.get_attribute('src').startswith('https:'):
    img.scroll_into_view_if_needed()
    page.wait_for_function('(i)=>i.complete&&i.naturalWidth>0',arg=img.element_handle())
  for a in page.locator('a[href^="#"]').all():
   target=a.get_attribute('href')[1:]
   assert not target or page.locator('[id="'+target+'"]').count(),target
  page.evaluate('window.scrollTo(0,0)')
  if width in [1440,390]:page.screenshot(path=str(Path(tempfile.gettempdir()) / f'volumefy-{width}.png'),full_page=True)
  page.goto((root/'docs/panel.html').as_uri())
  assert page.evaluate('document.documentElement.scrollWidth <= innerWidth'),f'panel overflow {width}'
 assert not errors,errors
 browser.close()
 print('PASS: filters, 125-profile UI, speed, demo action/upload isolation, image assets, anchors, layouts 1440/390/320, no JS errors')
