# Documentation Raytracer — site statique

Ce dossier (`docs/site/`) contient la documentation technique du Raytracer sous
forme de **site statique HTML**, sans aucune dépendance à un serveur : il
s'ouvre directement dans un navigateur et se déploie tel quel sur
**Cloudflare Pages**.

## Contenu

```
docs/site/
├── index.html            Présentation générale et aperçu
├── installation.html     Prérequis, dépendances, compilation
├── utilisation.html      Ligne de commande et format des scènes
├── architecture.html     Organisation du code et pipeline de rendu
├── fonctionnalites.html  Primitives, lumières, matériaux, édition…
├── reference.html        Interfaces et classes principales
├── faq.html              Problèmes courants et solutions
├── assets/
│   ├── style.css         Feuille de style commune
│   └── main.js           Menu mobile + coloration syntaxique
└── README.md             Ce fichier
```

La coloration syntaxique des blocs de code utilise **highlight.js** via CDN.
Tous les liens entre pages sont **relatifs** ; l'ouverture directe des fichiers
(`file://`) fonctionne.

## Prévisualiser en local

Ouvrez simplement `index.html` dans un navigateur, ou servez le dossier :

```bash
# Depuis la racine du dépôt
cd docs/site
python3 -m http.server 8000
# puis http://localhost:8000
```

## Déploiement sur Cloudflare Pages

Le site est **entièrement statique** : aucune étape de build n'est nécessaire.

### Option A — Tableau de bord Cloudflare (Git)

1. Poussez le dépôt sur GitHub/GitLab.
2. Dans le tableau de bord Cloudflare → **Workers & Pages** → **Create
   application** → **Pages** → **Connect to Git**, sélectionnez le dépôt.
3. Dans les réglages de build :
   - **Framework preset** : `None`
   - **Build command** : *(laisser vide)*
   - **Build output directory** : `docs/site`
4. **Save and Deploy**. Cloudflare publie le contenu de `docs/site` tel quel.

> Le dossier de sortie est `docs/site` (et non `docs`) car `docs/` contient par
> ailleurs la documentation Markdown du projet.

### Option B — Wrangler (ligne de commande)

```bash
npm install -g wrangler
wrangler pages deploy docs/site --project-name=raytracer-docs
```

### Option C — Upload direct

Dans **Workers & Pages** → **Create application** → **Pages** → **Upload
assets**, glissez-déposez le contenu du dossier `docs/site`.

## Notes

- Aucune commande de build, aucune variable d'environnement requise.
- Le contenu est rédigé à partir du code réel du projet ; les points restant à
  préciser sont marqués par des commentaires `<!-- À COMPLÉTER -->` dans le HTML
  (le cas échéant).
