// Petit script commun : menu mobile + coloration syntaxique (highlight.js).
(function () {
    "use strict";

    // Bascule du menu latéral sur mobile.
    document.addEventListener("click", function (e) {
        var toggle = e.target.closest("[data-menu-toggle]");
        if (toggle) {
            document.body.classList.toggle("nav-open");
            return;
        }
        // Ferme le menu quand on clique sur l'overlay ou un lien de la sidebar.
        if (document.body.classList.contains("nav-open")) {
            if (e.target.closest(".sidebar a") || !e.target.closest(".sidebar")) {
                document.body.classList.remove("nav-open");
            }
        }
    });

    // Coloration syntaxique si highlight.js (CDN) est disponible.
    if (window.hljs) {
        window.hljs.highlightAll();
    }
})();
