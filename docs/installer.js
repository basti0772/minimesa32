const status = document.querySelector('#status');
try {
  const responses = await Promise.all([
    fetch('manifest.json', { cache: 'no-cache' }),
    fetch('firmware/build-info.json', { cache: 'no-cache' })
  ]);
  if (responses.some(response => !response.ok)) throw new Error('Firmware-Metadaten nicht erreichbar.');
  const [manifest, info] = await Promise.all(responses.map(response => response.json()));
  if (manifest.version !== info.version || manifest.builds?.length !== 1 ||
      manifest.builds[0].chipFamily !== 'ESP32' || manifest.builds[0].parts?.length !== 1 ||
      manifest.builds[0].parts[0].offset !== 0 ||
      manifest.builds[0].parts[0].path !== 'firmware/minimesa32.bin') {
    throw new Error('Firmware-Metadaten sind inkonsistent.');
  }
  document.querySelector('#version').textContent = `Version ${manifest.version}`;
  document.querySelector('#build').textContent = `${info.flashSize} · ${info.flashMode.toUpperCase()} · ${info.flashFrequency.replace(/m$/, ' MHz')} · ${(info.mergedSize / 1024).toFixed(1)} KiB`;
  await import('https://unpkg.com/esp-web-tools@10.1.0/dist/web/install-button.js?module');
  await customElements.whenDefined('esp-web-install-button');
  document.querySelector('#installer').hidden = false;
  status.textContent = 'Bereit. Der Browser fragt erst nach einem Klick nach dem USB-Port.';
} catch (error) {
  if (document.querySelector('#version').textContent.includes('geladen')) {
    document.querySelector('#version').textContent = 'Version nicht verfügbar';
  }
  status.classList.add('error');
  status.textContent = `Installer nicht verfügbar: ${error.message} Bitte neu laden oder die manuelle Installation verwenden.`;
}
