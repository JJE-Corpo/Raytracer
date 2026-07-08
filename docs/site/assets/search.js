// Recherche client, sans serveur. L'index est fourni par assets/search-index.js
// sous forme de variable globale window.SEARCH_INDEX (compatible file://).
(function () {
    "use strict";

    var input = document.getElementById("search-input");
    var box = document.getElementById("search-results");
    if (!input || !box) return;

    var lang = (document.documentElement.getAttribute("lang") || "fr").slice(0, 2);
    var index = (window.SEARCH_INDEX || []).filter(function (e) { return e.lang === lang; });
    var active = -1;
    var current = [];

    var LABELS = {
        fr: { empty: "Aucun résultat", placeholder: "Rechercher…  (/)" },
        en: { empty: "No results", placeholder: "Search…  (/)" }
    };
    var L = LABELS[lang] || LABELS.fr;
    input.setAttribute("placeholder", L.placeholder);

    function norm(s) {
        return (s || "").toLowerCase()
            .normalize("NFD").replace(/[̀-ͯ]/g, ""); // enlève les accents
    }

    function escapeHtml(s) {
        return s.replace(/[&<>"']/g, function (c) {
            return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;" }[c];
        });
    }

    function snippet(text, q) {
        var nt = norm(text), nq = norm(q);
        var i = nt.indexOf(nq);
        if (i < 0) return escapeHtml(text.slice(0, 120));
        var start = Math.max(0, i - 40);
        var end = Math.min(text.length, i + q.length + 60);
        var pre = (start > 0 ? "…" : "") + text.slice(start, i);
        var mid = text.slice(i, i + q.length);
        var post = text.slice(i + q.length, end) + (end < text.length ? "…" : "");
        return escapeHtml(pre) + "<mark>" + escapeHtml(mid) + "</mark>" + escapeHtml(post);
    }

    function score(entry, nq) {
        var nTitle = norm(entry.title), nHead = norm(entry.headings || ""), nText = norm(entry.text || "");
        if (nTitle.indexOf(nq) === 0) return 100;
        if (nTitle.indexOf(nq) >= 0) return 70;
        if (nHead.indexOf(nq) >= 0) return 45;
        if (nText.indexOf(nq) >= 0) return 20;
        return 0;
    }

    function run(q) {
        var nq = norm(q.trim());
        if (nq.length < 2) { close(); return; }
        current = index.map(function (e) { return { e: e, s: score(e, nq) }; })
            .filter(function (r) { return r.s > 0; })
            .sort(function (a, b) { return b.s - a.s; })
            .slice(0, 12)
            .map(function (r) { return r.e; });
        render(q);
    }

    function render(q) {
        active = -1;
        if (!current.length) {
            box.innerHTML = '<div class="sr-empty">' + L.empty + '</div>';
            box.classList.add("open");
            return;
        }
        box.innerHTML = current.map(function (e) {
            var hay = norm(e.text).indexOf(norm(q)) >= 0 ? e.text : (e.headings || e.text || "");
            return '<a class="sr-item" href="' + e.url + '">' +
                '<div class="sr-crumb">' + escapeHtml(e.page) + '</div>' +
                '<div class="sr-title">' + escapeHtml(e.title) + '</div>' +
                '<div class="sr-snippet">' + snippet(hay, q) + '</div>' +
                '</a>';
        }).join("");
        box.classList.add("open");
    }

    function close() { box.classList.remove("open"); box.innerHTML = ""; active = -1; }

    function move(delta) {
        var items = box.querySelectorAll(".sr-item");
        if (!items.length) return;
        if (active >= 0) items[active].classList.remove("active");
        active = (active + delta + items.length) % items.length;
        items[active].classList.add("active");
        items[active].scrollIntoView({ block: "nearest" });
    }

    input.addEventListener("input", function () { run(input.value); });
    input.addEventListener("keydown", function (e) {
        if (e.key === "ArrowDown") { e.preventDefault(); move(1); }
        else if (e.key === "ArrowUp") { e.preventDefault(); move(-1); }
        else if (e.key === "Enter") {
            var items = box.querySelectorAll(".sr-item");
            if (active >= 0 && items[active]) { window.location.href = items[active].getAttribute("href"); }
            else if (items[0]) { window.location.href = items[0].getAttribute("href"); }
        } else if (e.key === "Escape") { close(); input.blur(); }
    });

    document.addEventListener("click", function (e) {
        if (!e.target.closest(".search")) close();
    });

    // Raccourci "/" pour focus la recherche.
    document.addEventListener("keydown", function (e) {
        if (e.key === "/" && document.activeElement !== input &&
            !/^(INPUT|TEXTAREA)$/.test(document.activeElement.tagName)) {
            e.preventDefault(); input.focus();
        }
    });
})();
