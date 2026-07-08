#!/usr/bin/env python3
"""Réécrit le bloc <aside class="sidebar">…</aside> de TOUTES les pages du site
avec la barre latérale canonique (FR ou EN selon le fichier), en marquant la
page courante `active`. Garantit une navigation identique sur toutes les pages."""
import os, re, glob

SITE = "/home/julien/raytracer_V2/docs/site"

# Groupes : (titre_fr, titre_en, [(base, label_fr, label_en), ...])
GROUPS = [
    ("Démarrer", "Get started", [
        ("index", "Accueil", "Home"),
        ("installation", "Installation", "Installation"),
        ("utilisation", "Utilisation", "Usage"),
    ]),
    ("Scène &amp; contenu", "Scene &amp; content", [
        ("format-scene", "Format de scène", "Scene format"),
        ("primitives", "Primitives", "Primitives"),
        ("materiaux", "Matériaux", "Materials"),
        ("fonctionnalites", "Fonctionnalités", "Features"),
    ]),
    ("Moteur", "Engine", [
        ("architecture", "Architecture", "Architecture"),
        ("rendu", "Pipeline de rendu", "Rendering pipeline"),
        ("plugins", "Système de plugins", "Plugin system"),
        ("cluster", "Rendu en cluster", "Cluster rendering"),
    ]),
    ("Interface", "Interface", [
        ("interface", "Interface graphique", "Graphical interface"),
    ]),
    ("Référence", "Reference", [
        ("reference", "Référence des classes", "Class reference"),
        ("faq", "FAQ", "FAQ"),
    ]),
]

def href(base, lang):
    return f"{base}.en.html" if lang == "en" else f"{base}.html"

def build_sidebar(cur_base, lang):
    out = ['<aside class="sidebar">']
    for gfr, gen, items in GROUPS:
        out.append(f"        <h4>{gen if lang=='en' else gfr}</h4>")
        out.append("        <ul>")
        for base, lfr, len_ in items:
            label = len_ if lang == "en" else lfr
            active = ' class="active"' if base == cur_base else ""
            out.append(f'            <li><a href="{href(base, lang)}"{active}>{label}</a></li>')
        out.append("        </ul>")
    out.append("    </aside>")
    return "\n    ".join(out)

def page_base_lang(fname):
    name = fname[:-5]
    if name.endswith(".en"):
        return name[:-3], "en"
    return name, "fr"

def main():
    pat = re.compile(r'<aside class="sidebar">.*?</aside>', re.S)
    changed = 0
    for path in sorted(glob.glob(os.path.join(SITE, "*.html"))):
        fname = os.path.basename(path)
        base, lang = page_base_lang(fname)
        s = open(path, encoding="utf-8").read()
        if '<aside class="sidebar">' not in s:
            print(f"  !! pas de sidebar dans {fname}")
            continue
        new = build_sidebar(base, lang)
        s2, n = pat.subn(lambda _: new, s, count=1)
        if n and s2 != s:
            open(path, "w", encoding="utf-8").write(s2)
            changed += 1
    print(f"Sidebar normalisée sur {changed} page(s).")

if __name__ == "__main__":
    main()
