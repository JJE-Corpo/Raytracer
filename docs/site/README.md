# Documentation Raytracer — site statique

Ce dossier (`docs/site/`) contient la documentation technique du Raytracer sous
forme de **site statique HTML**, sans aucune dépendance à un serveur : il
s'ouvre directement dans un navigateur et se déploie tel quel sur
**Cloudflare Pages**.

Le site est **bilingue (français / anglais)** et embarque une **barre de
recherche** entièrement côté client.

## Contenu

```
docs/site/
├── index.html / index.en.html                 Présentation et aperçu
├── installation.html / .en.html               Prérequis, dépendances, compilation
├── utilisation.html / .en.html                Ligne de commande et format des scènes
├── architecture.html / .en.html               Organisation du code, outils, pipeline, cluster
├── fonctionnalites.html / .en.html            Primitives, lumières, matériaux, BVH, maillages
├── interface.html / .en.html                  Panneaux, composants, gizmos, édition de sommets
├── reference.html / .en.html                  Interfaces, classes, types mathématiques
├── faq.html / .en.html                        Problèmes courants et solutions
├── assets/
│   ├── style.css              Feuille de style commune
│   ├── main.js                Menu mobile + coloration syntaxique
│   ├── search.js              Moteur de recherche côté client
│   └── search-index.js        Index de recherche (généré, FR + EN)
├── tools/
│   └── build_search_index.py  Régénère assets/search-index.js à partir des pages
└── README.md                  Ce fichier
```

- Les pages françaises sont les fichiers `*.html`, les pages anglaises sont les
  fichiers `*.en.html` (mêmes noms d'ancres, pour que la bascule de langue
  conserve la position dans la page).
- La bascule **🌐 FR / EN** est en haut à droite de chaque page.
- La coloration syntaxique des blocs de code utilise **highlight.js** via CDN.
- Tous les liens entre pages sont **relatifs** ; l'ouverture directe des
  fichiers (`file://`) fonctionne, y compris la recherche (l'index est chargé
  comme un script, sans requête réseau).

## Barre de recherche

La recherche est **statique** : `assets/search.js` interroge l'index
`assets/search-index.js` (une variable JavaScript globale), filtré par la langue
de la page courante. Raccourci clavier : `/` pour focaliser le champ.

### Régénérer l'index après avoir modifié le contenu

L'index est produit automatiquement en scannant toutes les pages HTML :

```bash
cd docs/site
python3 tools/build_search_index.py
```

Le script (sans dépendance externe) crée une entrée par section `<h2>`/`<h3>`
avec son ancre, pour les versions FR et EN. À relancer après toute modification
du texte ou des titres.

## Prévisualiser en local

Ouvrez simplement `index.html` dans un navigateur, ou servez le dossier :

```bash
cd docs/site
python3 -m http.server 8000
# puis http://localhost:8000
```

## Déploiement sur Cloudflare Pages

Le site est **entièrement statique** : aucune étape de build n'est nécessaire
côté Cloudflare (seule la régénération de l'index, ci-dessus, est à faire
localement si le contenu change).

### Option A — Tableau de bord Cloudflare (Git)

1. Poussez le dépôt sur GitHub/GitLab.
2. Dans le tableau de bord Cloudflare → **Workers & Pages** → **Create
   application** → **Pages** → **Connect to Git**, sélectionnez le dépôt.
3. Dans les réglages de build :
   - **Framework preset** : `None`
   - **Build command** : *(laisser vide)*
   - **Build output directory** : `docs/site`
4. **Save and Deploy**.

> Le dossier de sortie est `docs/site` (et non `docs`) car `docs/` contient par
> ailleurs la documentation Markdown du projet.

### Option B — Wrangler (ligne de commande)

```bash
npm install -g wrangler
wrangler pages deploy docs/site --project-name=raytracer-docs --branch main
```

### Option C — Upload direct

Dans **Workers & Pages** → **Create application** → **Pages** → **Upload
assets**, glissez-déposez le contenu du dossier `docs/site`.

## Notes

- Aucune variable d'environnement requise.
- Le contenu est rédigé à partir du code réel du projet (Makefile, en-têtes,
  parseur de scène, exemples de `tests/configs/`, et la documentation Markdown
  du dossier `docs/`).
- Site en ligne : <https://raytracer-docs.pages.dev/>
