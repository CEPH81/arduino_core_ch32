#!/usr/bin/env python3
"""
paquete.py - paquete del Gestor de placas (Arduino IDE 2.x) del fork de CEPH81 de
arduino_core_ch32 (CH32V de openwch + CH57x/CH58x/CH59x).

Genera en dist/:
  - ch32v_ceph81-<version>.tar.bz2      la plataforma
  - package_ch32v_ceph81_index.json     indice (plataforma + compilador, openocd y beforeinstall
                                        de WCH, con las mismas descargas que el indice de openwch)

Publicacion:
  1. python3 tools/paquete.py
  2. subir el .tar.bz2 a la rama "paquetes"
  3. copiar el .json a la raiz de la rama ch5xx y hacer push
  4. URL del gestor de placas:
     https://raw.githubusercontent.com/CEPH81/arduino_core_ch32/ch5xx/package_ch32v_ceph81_index.json
"""
import bz2, hashlib, io, json, os, re, sys, tarfile

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DIST = os.path.join(RAIZ, 'dist')
REPO = 'CEPH81/arduino_core_ch32'
PAQUETE = 'ch32v_ceph81'
PACKAGER = 'ceph81'
FECHA = 1767225600  # 2026-01-01
BASE = os.environ.get('CH32V_URL', 'https://raw.githubusercontent.com/%s/paquetes/' % REPO)

INCLUIR = ['boards.txt', 'platform.txt', 'programmers.txt', 'README.md', 'README_CH5xx.md',
           'package.json', 'cores', 'variants', 'system', 'libraries', 'tools/wchisp']
EXCLUIR = re.compile(r'(^|/)(platform\.local\.txt|\.git[^/]*|__pycache__|[^/]*\.(o|d|pyc))(/|$)')

def herramientas():
    """Toma las entradas exactas del indice de openwch (board_manager_files)."""
    ruta = os.environ.get('WCH_INDEX', '/home/claude/wch/board_manager_files/package_ch32v_index.json')
    d = json.load(open(ruta, encoding='utf-8'))
    return [t for t in d['packages'][0]['tools']
            if (t['name'], t['version']) in {('riscv-none-embed-gcc', '8.2.0'), ('openocd', '1.0.0'), ('beforeinstall', '1.0.0')}]


def leer(clave):
    for l in open(os.path.join(RAIZ, 'platform.txt'), encoding='utf-8'):
        if l.startswith(clave + '='):
            return l.split('=', 1)[1].strip()
    sys.exit('platform.txt sin ' + clave)


def placas():
    return [{"name": m.group(1)} for m in
            re.finditer(r'^\w+\.name=(.+)$', open(os.path.join(RAIZ, 'boards.txt'), encoding='utf-8').read(), re.M)]


def crear_tar(version):
    nombre = '%s-%s.tar.bz2' % (PAQUETE, version)
    tope = '%s-%s' % (PAQUETE, version)
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode='w', format=tarfile.GNU_FORMAT) as tar:
        archivos = []
        for inc in INCLUIR:
            ruta = os.path.join(RAIZ, inc)
            if os.path.isfile(ruta):
                archivos.append(inc)
            else:
                for r, ds, fs in os.walk(ruta):
                    ds.sort()
                    for f in sorted(fs):
                        archivos.append(os.path.relpath(os.path.join(r, f), RAIZ).replace(os.sep, '/'))
        for rel in sorted(archivos):
            if EXCLUIR.search(rel):
                continue
            ruta = os.path.join(RAIZ, rel)
            info = tar.gettarinfo(ruta, arcname=tope + '/' + rel)
            info.uid = info.gid = 0
            info.uname = info.gname = ''
            info.mtime = FECHA
            info.mode = 0o755 if rel.startswith('tools/wchisp/') and not rel.endswith('.md') else 0o644
            with open(ruta, 'rb') as f:
                tar.addfile(info, f)
    datos = bz2.compress(buf.getvalue(), 9)
    os.makedirs(DIST, exist_ok=True)
    open(os.path.join(DIST, nombre), 'wb').write(datos)
    return nombre, len(datos), hashlib.sha256(datos).hexdigest()


def main():
    version = leer('version')
    nombre, tam, sha = crear_tar(version)
    tools = json.loads(os.environ['TOOLS_JSON']) if 'TOOLS_JSON' in os.environ else herramientas()
    web = "https://github.com/%s/tree/ch5xx" % REPO
    indice = {"packages": [{
        "name": PACKAGER,
        "maintainer": "MikrocontrollerCephs (CEPH81) - fork de openwch/arduino_core_ch32",
        "websiteURL": web, "email": "", "help": {"online": web},
        "platforms": [{
            "name": leer('name'), "architecture": "ch32v", "version": version,
            "category": "Contributed", "url": BASE + nombre, "archiveFileName": nombre,
            "checksum": "SHA-256:" + sha, "size": str(tam), "help": {"online": web},
            "boards": placas(),
            "toolsDependencies": [{"packager": PACKAGER, "name": t["name"], "version": t["version"]} for t in tools],
        }],
        "tools": tools,
    }]}
    ruta = os.path.join(DIST, 'package_%s_index.json' % PAQUETE)
    with open(ruta, 'w', encoding='utf-8') as f:
        json.dump(indice, f, indent=2, ensure_ascii=False)
        f.write('\n')
    print('Plataforma %s: %s (%d bytes, SHA-256 %s)' % (version, nombre, tam, sha))
    print('Indice: %s' % ruta)


if __name__ == '__main__':
    main()
