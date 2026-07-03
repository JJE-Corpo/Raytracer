//
// Created by jazema on 5/1/26.
//

#ifndef TEXTFIELD_HPP
#define TEXTFIELD_HPP
#include <SFML/Graphics.hpp>
#include <SFML/Window/Clipboard.hpp>
#include <algorithm>
#include <cctype>
#include <functional>

#include "../Component.hpp"
#include "../Theme.hpp"

namespace rc
{
    struct TextField: Component
    {
        mutable sf::RectangleShape box;
        sf::Text text;

        mutable sf::RectangleShape caret;
        mutable sf::RectangleShape selectionHighlight;
        sf::Clock caretClock;

        std::string value;
        bool focused = false;

        std::function<bool(const std::string&)> onType = nullptr;
        std::function<bool(const std::string&)> onValidate = nullptr;

        bool caretVisible = true;
        size_t cursorPos = 0;

        // The selection is the range [selMin(), selMax()) of value. The anchor is
        // the fixed end (set where the drag/extend started); cursorPos is the
        // moving end. When they are equal there is no selection, only a caret.
        size_t selectionAnchor = 0;
        // True while the left button is held after a press inside the box, so
        // MouseMoved events extend the selection (drag-select).
        bool selecting = false;

        size_t getCursorPosFromMouse(float mouseX) const
        {
            if (this->value.empty())
                return 0;

            float startX = this->text.getPosition().x;
            if (mouseX <= startX)
                return 0;

            size_t maxIndex = this->value.size();
            for (size_t i = 0; i < maxIndex; ++i)
            {
                float leftX = this->text.findCharacterPos(i).x;
                float rightX = this->text.findCharacterPos(i + 1).x;
                float midX = (leftX + rightX) * 0.5f;
                if (mouseX < midX)
                    return i;
            }
            return maxIndex;
        }

        void setFont(sf::Font &font) override
        {
            this->text.setFont(font);
            this->text.setCharacterSize(16);
            this->text.setFillColor(theme::TEXT_WHITE);
            this->caret.setSize({1.f, static_cast<float>(this->text.getCharacterSize())});
            this->caret.setFillColor(theme::TEXT_WHITE);
        }

        void setCharacterSize(int size)
        {
            this->text.setCharacterSize(size);
            this->caret.setSize({1.f, static_cast<float>(this->text.getCharacterSize())});
        }

        void layout(float x, float y, float w, float h)
        {
            this->box.setPosition(x, y);
            this->box.setSize({w, h});
            float padding = 8.f;
            float textY = y + (h - this->text.getCharacterSize()) / 2.f - 2.f;
            this->text.setPosition(x + padding, textY);
        }

        void setValue(const std::string &v)
        {
            this->value = v;
            this->cursorPos = this->value.size();
            this->selectionAnchor = this->cursorPos;
            this->text.setString(this->value);
        }

        // --- selection helpers ---------------------------------------------

        bool hasSelection() const
        {
            return (this->selectionAnchor != this->cursorPos);
        }

        size_t selMin() const
        {
            return (std::min(this->selectionAnchor, this->cursorPos));
        }

        size_t selMax() const
        {
            return (std::max(this->selectionAnchor, this->cursorPos));
        }

        std::string selectedText() const
        {
            if (!this->hasSelection())
                return (std::string());
            return (this->value.substr(this->selMin(), this->selMax() - this->selMin()));
        }

        // A "word" is a run of alphanumeric/underscore characters; everything
        // else (spaces, punctuation, path separators) counts as a boundary.
        static bool isWordChar(char c)
        {
            unsigned char uc = static_cast<unsigned char>(c);
            return (std::isalnum(uc) || c == '_');
        }

        // Start of the word to the left of pos (Ctrl+Left / Ctrl+Backspace
        // target): skip any boundary characters, then the word itself.
        size_t prevWordBoundary(size_t pos) const
        {
            size_t i = pos;
            while (i > 0 && !isWordChar(this->value[i - 1]))
                i--;
            while (i > 0 && isWordChar(this->value[i - 1]))
                i--;
            return (i);
        }

        // Start of the next word to the right of pos (Ctrl+Right / Ctrl+Delete
        // target): skip the current word, then the boundary characters after it.
        size_t nextWordBoundary(size_t pos) const
        {
            size_t n = this->value.size();
            size_t i = pos;
            while (i < n && isWordChar(this->value[i]))
                i++;
            while (i < n && !isWordChar(this->value[i]))
                i++;
            return (i);
        }

        // Blink reset: force the caret on and restart its half-second clock so it
        // stays solid right after any cursor/selection change.
        void resetCaret()
        {
            this->caretClock.restart();
            this->caretVisible = true;
        }

        // Splice insert into value in place of [from, to), if the resulting text
        // passes the onType validator. Collapses the selection onto the caret at
        // the end of the inserted text. Returns whether the edit was applied.
        bool replaceRange(size_t from, size_t to, const std::string &insert)
        {
            std::string newValue = this->value;
            newValue.erase(from, to - from);
            newValue.insert(from, insert);
            bool allowed = !this->onType || this->onType(newValue);
            if (!allowed)
                return (false);
            this->value = std::move(newValue);
            this->cursorPos = from + insert.size();
            this->selectionAnchor = this->cursorPos;
            this->text.setString(this->value);
            this->resetCaret();
            return (true);
        }

        bool deleteSelection()
        {
            if (!this->hasSelection())
                return (false);
            return (this->replaceRange(this->selMin(), this->selMax(), std::string()));
        }

        void pasteFromClipboard()
        {
            std::string clip = sf::Clipboard::getString().toAnsiString();
            // Keep only printable ASCII: a single-line field must never ingest
            // the newlines/control characters a clipboard may carry.
            std::string filtered;
            filtered.reserve(clip.size());
            for (char c : clip)
            {
                unsigned char uc = static_cast<unsigned char>(c);
                if (uc >= 32 && uc < 127)
                    filtered += c;
            }
            if (filtered.empty())
                return;
            this->replaceRange(this->selMin(), this->selMax(), filtered);
        }

        void update(sf::Vector2i mouse) override
        {
            this->hovered = this->box.getGlobalBounds().contains(static_cast<float>(mouse.x), static_cast<float>(mouse.y));
            if (!this->enabled)
                return;
            if (this->caretClock.getElapsedTime().asSeconds() > 0.5f)
            {
                this->caretVisible = !this->caretVisible;
                this->caretClock.restart();
            }
        }

        sf::FloatRect getBounds() const override
        {
            return (this->box.getGlobalBounds());
        }

        // A focused field captures keyboard events wherever the cursor is. It also
        // keeps receiving pointer events (MouseMoved) so a drag-select can extend
        // past the box edge.
        bool isCapturing() const override
        {
            return (this->focused && this->enabled);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->enabled)
                return (false);

            // --- mouse: press/drag/release also drive focus ----------------
            // Focus used to be applied from mouse polling in update(); it is done
            // here instead so a press cleanly starts a drag-select and a release
            // ends it, rather than the poll re-placing the caret every frame.
            if (event.type == sf::Event::MouseButtonPressed
                && event.mouseButton.button == sf::Mouse::Left)
            {
                const bool inside = this->box.getGlobalBounds().contains((float)mouse.x, (float)mouse.y);
                if (inside)
                {
                    const bool wasFocused = this->focused;
                    const size_t pos = this->getCursorPosFromMouse((float)mouse.x);
                    const bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)
                        || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
                    this->focused = true;
                    this->cursorPos = pos;
                    // Shift+click on an already-focused field extends from the
                    // current anchor; any other click collapses to a caret.
                    if (!(shift && wasFocused))
                        this->selectionAnchor = pos;
                    this->selecting = true;
                    this->resetCaret();
                    return (true);
                }
                // A click elsewhere drops focus so the field stops eating the
                // keyboard, but is not consumed: it must reach what is underneath.
                this->focused = false;
                this->selecting = false;
                return (false);
            }

            if (event.type == sf::Event::MouseButtonReleased
                && event.mouseButton.button == sf::Mouse::Left)
            {
                const bool wasSelecting = this->selecting;
                this->selecting = false;
                return (wasSelecting);
            }

            if (event.type == sf::Event::MouseMoved && this->selecting)
            {
                // Extend the selection: move the cursor, leave the anchor put.
                this->cursorPos = this->getCursorPosFromMouse((float)mouse.x);
                this->resetCaret();
                return (true);
            }

            if (!this->focused)
                return (false);

            if (event.type == sf::Event::TextEntered)
            {
                // Backspace and Delete are handled in KeyPressed below so their
                // Ctrl modifier can trigger word deletion; here we only insert
                // printable characters (replacing any active selection).
                if (event.text.unicode < 128 && std::isprint(static_cast<int>(event.text.unicode)))
                {
                    char ch = static_cast<char>(event.text.unicode);
                    this->replaceRange(this->selMin(), this->selMax(), std::string(1, ch));
                }
                return (true);
            }

            if (event.type == sf::Event::KeyPressed)
            {
                const bool shift = event.key.shift;

                // --- clipboard / select-all shortcuts ----------------------
                if (event.key.control)
                {
                    if (event.key.code == sf::Keyboard::A)
                    {
                        this->selectionAnchor = 0;
                        this->cursorPos = this->value.size();
                        this->resetCaret();
                        return (true);
                    }
                    if (event.key.code == sf::Keyboard::C)
                    {
                        if (this->hasSelection())
                            sf::Clipboard::setString(sf::String(this->selectedText()));
                        return (true);
                    }
                    if (event.key.code == sf::Keyboard::X)
                    {
                        if (this->hasSelection())
                        {
                            sf::Clipboard::setString(sf::String(this->selectedText()));
                            this->deleteSelection();
                        }
                        return (true);
                    }
                    if (event.key.code == sf::Keyboard::V)
                    {
                        this->pasteFromClipboard();
                        return (true);
                    }
                }

                if (event.key.code == sf::Keyboard::Return)
                {
                    if (this->onValidate)
                    {
                        bool allowed = this->onValidate(this->value);
                        if (!allowed)
                            return (true);
                        this->focused = false;
                    }
                    return (true);
                }

                if (event.key.code == sf::Keyboard::Left)
                {
                    // Ctrl jumps a whole word; plain Left over a selection
                    // collapses to its left edge; otherwise it steps one char.
                    if (event.key.control)
                        this->cursorPos = this->prevWordBoundary(this->cursorPos);
                    else if (this->hasSelection() && !shift)
                        this->cursorPos = this->selMin();
                    else if (this->cursorPos > 0)
                        this->cursorPos--;
                    if (!shift)
                        this->selectionAnchor = this->cursorPos;
                    this->resetCaret();
                    return (true);
                }

                if (event.key.code == sf::Keyboard::Right)
                {
                    if (event.key.control)
                        this->cursorPos = this->nextWordBoundary(this->cursorPos);
                    else if (this->hasSelection() && !shift)
                        this->cursorPos = this->selMax();
                    else if (this->cursorPos < this->value.size())
                        this->cursorPos++;
                    if (!shift)
                        this->selectionAnchor = this->cursorPos;
                    this->resetCaret();
                    return (true);
                }

                if (event.key.code == sf::Keyboard::Home)
                {
                    this->cursorPos = 0;
                    if (!shift)
                        this->selectionAnchor = this->cursorPos;
                    this->resetCaret();
                    return (true);
                }

                if (event.key.code == sf::Keyboard::End)
                {
                    this->cursorPos = this->value.size();
                    if (!shift)
                        this->selectionAnchor = this->cursorPos;
                    this->resetCaret();
                    return (true);
                }

                if (event.key.code == sf::Keyboard::BackSpace)
                {
                    // Ctrl+Backspace deletes the whole word to the left.
                    if (this->hasSelection())
                        this->deleteSelection();
                    else if (event.key.control)
                    {
                        size_t start = this->prevWordBoundary(this->cursorPos);
                        if (start < this->cursorPos)
                            this->replaceRange(start, this->cursorPos, std::string());
                    }
                    else if (this->cursorPos > 0)
                        this->replaceRange(this->cursorPos - 1, this->cursorPos, std::string());
                    return (true);
                }

                if (event.key.code == sf::Keyboard::Delete)
                {
                    // Ctrl+Delete deletes the whole word to the right.
                    if (this->hasSelection())
                        this->deleteSelection();
                    else if (event.key.control)
                    {
                        size_t end = this->nextWordBoundary(this->cursorPos);
                        if (end > this->cursorPos)
                            this->replaceRange(this->cursorPos, end, std::string());
                    }
                    else if (this->cursorPos < this->value.size())
                        this->replaceRange(this->cursorPos, this->cursorPos + 1, std::string());
                    return (true);
                }
                return (true);
            }

            return (false);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            this->box.setFillColor(!this->enabled ? theme::BG_DISABLED
                                     : this->focused ? theme::BG_CONTROL_HOVER
                                               : theme::BG_CONTROL);

            target.draw(this->box, states);

            // Selection highlight sits behind the glyphs so the text stays legible.
            if (this->focused && this->enabled && this->hasSelection())
            {
                float x0 = this->text.findCharacterPos(this->selMin()).x;
                float x1 = this->text.findCharacterPos(this->selMax()).x;
                this->selectionHighlight.setPosition(x0, this->text.getPosition().y);
                this->selectionHighlight.setSize({x1 - x0, static_cast<float>(this->text.getCharacterSize())});
                this->selectionHighlight.setFillColor(theme::SELECTION_BG);
                target.draw(this->selectionHighlight, states);
            }

            target.draw(this->text, states);
            if (this->focused && this->caretVisible && this->enabled)
            {
                sf::Vector2f caretPos = this->text.findCharacterPos(this->cursorPos);

                this->caret.setPosition(
                    caretPos.x,
                    this->text.getPosition().y
                );
                target.draw(this->caret, states);
            }
        }

        CursorType getCursor() override
        {
            if (!this->enabled && this->hovered)
                return (CursorType::NOT_ALLOWED);
            if (this->hovered)
                return (CursorType::TEXT);
            return (CursorType::ARROW);
        }
    };
}

#endif
