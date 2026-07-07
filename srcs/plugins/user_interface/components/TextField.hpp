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

        static constexpr float PADDING = 8.f;

        mutable float scrollX = 0.f;

        size_t selectionAnchor = 0;
        bool selecting = false;

        size_t getCursorPosFromMouse(float mouseX) const
        {
            if (this->value.empty())
                return 0;

            float adjustedX = mouseX + this->scrollX;
            float startX = this->text.getPosition().x;
            if (adjustedX <= startX)
                return 0;

            size_t maxIndex = this->value.size();
            for (size_t i = 0; i < maxIndex; ++i)
            {
                float leftX = this->text.findCharacterPos(i).x;
                float rightX = this->text.findCharacterPos(i + 1).x;
                float midX = (leftX + rightX) * 0.5f;
                if (adjustedX < midX)
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
            float textY = y + (h - this->text.getCharacterSize()) / 2.f - 2.f;
            this->text.setPosition(x + PADDING, textY);
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

        static bool isWordChar(char c)
        {
            unsigned char uc = static_cast<unsigned char>(c);
            return (std::isalnum(uc) || c == '_');
        }

        size_t prevWordBoundary(size_t pos) const
        {
            size_t i = pos;
            while (i > 0 && !isWordChar(this->value[i - 1]))
                i--;
            while (i > 0 && isWordChar(this->value[i - 1]))
                i--;
            return (i);
        }

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

        void resetCaret()
        {
            this->caretClock.restart();
            this->caretVisible = true;
        }

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

        bool isCapturing() const override
        {
            return (this->focused && this->enabled);
        }

        bool handleEvent(const sf::Event &event, const sf::Vector2i mouse) override
        {
            if (!this->enabled)
                return (false);

            // --- mouse: press/drag/release also drive focus ----------------
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
                    if (!(shift && wasFocused))
                        this->selectionAnchor = pos;
                    this->selecting = true;
                    this->resetCaret();
                    return (true);
                }
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
                this->cursorPos = this->getCursorPosFromMouse((float)mouse.x);
                this->resetCaret();
                return (true);
            }

            if (!this->focused)
                return (false);

            if (event.type == sf::Event::TextEntered)
            {
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

        float innerWidth() const
        {
            return (std::max(0.f, this->box.getSize().x - 2.f * PADDING));
        }

        void updateScroll() const
        {
            const float inner = this->innerWidth();
            const float textLeft = this->text.getPosition().x;
            const float textWidth = this->value.empty() ? 0.f
                : this->text.findCharacterPos(this->value.size()).x - textLeft;

            if (!this->focused || textWidth <= inner)
            {
                this->scrollX = 0.f;
                return;
            }

            const float margin = 2.f;
            const float caret = this->text.findCharacterPos(this->cursorPos).x - textLeft;
            if (caret < this->scrollX + margin)
                this->scrollX = caret - margin;
            else if (caret > this->scrollX + inner - margin)
                this->scrollX = caret - inner + margin;

            this->scrollX = std::clamp(this->scrollX, 0.f, textWidth - inner + margin);
        }

        bool beginClip(sf::RenderTarget &target, const sf::FloatRect &interior, const sf::View &saved) const
        {
            if (interior.width <= 0.f || interior.height <= 0.f)
                return (false);

            const sf::Vector2u ts = target.getSize();
            if (ts.x == 0 || ts.y == 0)
                return (false);
            const float W = static_cast<float>(ts.x);
            const float H = static_cast<float>(ts.y);

            const sf::Vector2i tl = target.mapCoordsToPixel({interior.left, interior.top}, saved);
            const sf::Vector2i br = target.mapCoordsToPixel(
                {interior.left + interior.width, interior.top + interior.height}, saved);
            const float rx = static_cast<float>(tl.x);
            const float ry = static_cast<float>(tl.y);
            const float rw = static_cast<float>(br.x - tl.x);
            const float rh = static_cast<float>(br.y - tl.y);
            if (rw <= 0.f || rh <= 0.f)
                return (false);

            const sf::FloatRect vp = saved.getViewport();
            const float ix0 = std::max(rx, vp.left * W);
            const float iy0 = std::max(ry, vp.top * H);
            const float ix1 = std::min(rx + rw, (vp.left + vp.width) * W);
            const float iy1 = std::min(ry + rh, (vp.top + vp.height) * H);
            if (ix1 <= ix0 || iy1 <= iy0)
                return (false);

            const float worldLeft = interior.left + this->scrollX;
            const float wl = worldLeft + (ix0 - rx) / rw * interior.width;
            const float wt = interior.top + (iy0 - ry) / rh * interior.height;
            const float ww = (ix1 - ix0) / rw * interior.width;
            const float wh = (iy1 - iy0) / rh * interior.height;

            sf::View clip;
            clip.setSize(ww, wh);
            clip.setCenter(wl + ww * 0.5f, wt + wh * 0.5f);
            clip.setViewport(sf::FloatRect(ix0 / W, iy0 / H, (ix1 - ix0) / W, (iy1 - iy0) / H));
            target.setView(clip);
            return (true);
        }

        void draw(sf::RenderTarget &target, sf::RenderStates states) const override
        {
            this->box.setFillColor(!this->enabled ? theme::BG_DISABLED
                                     : this->focused ? theme::BG_CONTROL_HOVER
                                               : theme::BG_CONTROL);

            target.draw(this->box, states);

            this->updateScroll();

            const sf::Vector2f boxPos = this->box.getPosition();
            const sf::Vector2f boxSize = this->box.getSize();
            const sf::FloatRect interior(boxPos.x + PADDING, boxPos.y,
                std::max(0.f, boxSize.x - 2.f * PADDING), boxSize.y);

            const sf::View saved = target.getView();
            const bool clipped = this->beginClip(target, interior, saved);
            if (!clipped)
                return;

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

            target.setView(saved);
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
