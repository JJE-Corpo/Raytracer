#!/usr/bin/env python3
"""Génère docs/site/assets/search-index.js à partir des pages HTML.
Une entrée par section <h2>/<h3> (avec ancre) + une entrée pour la page.
Aucune dépendance externe (parseur HTML léger via html.parser)."""
import os, re, json, html
from html.parser import HTMLParser

SITE = "/home/julien/raytracer_V2/docs/site"

# Titre lisible de chaque page (breadcrumb dans les résultats)
PAGE_TITLES = {
    "index":          ("Accueil", "Home"),
    "installation":   ("Installation", "Installation"),
    "utilisation":    ("Utilisation", "Usage"),
    "format-scene":   ("Format de scène", "Scene format"),
    "primitives":     ("Primitives", "Primitives"),
    "materiaux":      ("Matériaux", "Materials"),
    "fonctionnalites":("Fonctionnalités", "Features"),
    "architecture":   ("Architecture", "Architecture"),
    "rendu":          ("Pipeline de rendu", "Rendering pipeline"),
    "plugins":        ("Système de plugins", "Plugin system"),
    "cluster":        ("Rendu en cluster", "Cluster rendering"),
    "interface":      ("Interface graphique", "Graphical interface"),
    "reference":      ("Référence des classes", "Class reference"),
    "faq":            ("FAQ", "FAQ"),
}

VOID = {"input", "img", "br", "hr", "meta", "link", "source", "area",
        "base", "col", "embed", "param", "track", "wbr"}

class Extractor(HTMLParser):
    def __init__(self):
        super().__init__()
        self.skip = False             # dans une zone non-contenu ?
        self.depth = 0                # profondeur dans la zone ignorée courante
        self.cur_h = None             # ('h2'|'h3', id)
        self.h_text = []
        self.sections = []            # [(level, id, title)]
        self.body = []                # texte courant accumulé pour la page

    def _is_skip_region(self, tag, cls):
        return (tag in ("script", "style")
                or (tag == "aside" and "sidebar" in cls)
                or (tag == "header" and "topbar" in cls)
                or tag == "footer"
                or (tag == "nav" and "toc" in cls))

    def handle_starttag(self, tag, attrs):
        a = dict(attrs)
        cls = a.get("class", "")
        if self.skip:
            if tag not in VOID:
                self.depth += 1
            return
        if self._is_skip_region(tag, cls):
            self.skip = True
            self.depth = 1
            return
        if tag in ("h2", "h3") and a.get("id"):
            self.cur_h = (tag, a["id"]); self.h_text = []

    def handle_startendtag(self, tag, attrs):
        # balises auto-fermantes explicites (<input/>) : ne change pas la profondeur
        pass

    def handle_endtag(self, tag):
        if self.skip:
            if tag not in VOID:
                self.depth -= 1
                if self.depth <= 0:
                    self.skip = False
            return
        if tag in ("h2", "h3") and self.cur_h:
            title = re.sub(r"\s+", " ", "".join(self.h_text)).strip()
            if title:
                self.sections.append((self.cur_h[0], self.cur_h[1], title))
            self.cur_h = None; self.h_text = []

    def handle_data(self, data):
        if self.skip:
            return
        if self.cur_h is not None:
            self.h_text.append(data)
        self.body.append(data)

def page_lang_base(fname):
    # renvoie (base, lang) : "architecture.en.html" -> ("architecture","en")
    name = fname[:-5]  # sans .html
    if name.endswith(".en"):
        return name[:-3], "en"
    return name, "fr"

def clean_text(parts):
    t = html.unescape("".join(parts))
    return re.sub(r"\s+", " ", t).strip()

def main():
    entries = []
    for fname in sorted(os.listdir(os.path.join(SITE))):
        if not fname.endswith(".html"):
            continue
        base, lang = page_lang_base(fname)
        if base not in PAGE_TITLES:
            continue
        with open(os.path.join(SITE, fname), encoding="utf-8") as f:
            src = f.read()
        # h1 / lead pour l'entrée de page
        m_h1 = re.search(r"<h1[^>]*>(.*?)</h1>", src, re.S)
        h1 = clean_text([re.sub(r"<[^>]+>", " ", m_h1.group(1))]) if m_h1 else PAGE_TITLES[base][0 if lang=="fr" else 1]
        m_lead = re.search(r'<p class="lead"[^>]*>(.*?)</p>', src, re.S)
        lead = clean_text([re.sub(r"<[^>]+>", " ", m_lead.group(1))]) if m_lead else ""

        p = Extractor(); p.feed(src)
        page_label = PAGE_TITLES[base][0 if lang == "fr" else 1]
        fulltext = clean_text(p.body)

        # Entrée "page"
        entries.append({
            "lang": lang, "page": page_label, "title": h1,
            "url": fname, "headings": " · ".join(s[2] for s in p.sections),
            "text": (lead + " " + fulltext)[:600],
        })
        # Entrées "section"
        # Répartit le texte par section en re-scannant les ancres dans l'ordre
        anchors = [s[1] for s in p.sections]
        for (lvl, sid, title) in p.sections:
            # extrait un extrait de texte après cette ancre
            seg = ""
            mm = re.search(r'id=["\']' + re.escape(sid) + r'["\']', src)
            if mm:
                after = src[mm.end():]
                after = re.sub(r"<(script|style)[^>]*>.*?</\1>", " ", after, flags=re.S)
                after = re.sub(r"<[^>]+>", " ", after)
                seg = clean_text([after])[:300]
            entries.append({
                "lang": lang, "page": page_label, "title": title,
                "url": fname + "#" + sid, "headings": title, "text": seg,
            })

    out = os.path.join(SITE, "assets", "search-index.js")
    with open(out, "w", encoding="utf-8") as f:
        f.write("// Généré automatiquement — index de recherche (FR + EN).\n")
        f.write("window.SEARCH_INDEX = ")
        json.dump(entries, f, ensure_ascii=False, separators=(",", ":"))
        f.write(";\n")
    fr = sum(1 for e in entries if e["lang"] == "fr")
    en = sum(1 for e in entries if e["lang"] == "en")
    print(f"Index écrit: {out}")
    print(f"  {len(entries)} entrées ({fr} FR, {en} EN)")

if __name__ == "__main__":
    main()
