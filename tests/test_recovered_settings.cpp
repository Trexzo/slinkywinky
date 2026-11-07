#include "slinkywinky/recovered_settings.hpp"
#include "slinkywinky/menu.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <set>
#include <stdexcept>
#include <string_view>

#define CHECK(x) do {if(!(x))throw std::runtime_error("R116 recovered settings: " #x);}while(false)
int main() {
    using namespace slinkywinky;
    const auto& items=recovered_widget_catalog();
    const auto& catalog=official_module_catalog();
    CHECK(items.size()==378);
    CHECK(catalog.size()==67);
    std::array<std::size_t,8> counts{};
    std::set<std::string_view> names;
    for(const auto& module:catalog)names.insert(module.name);
    std::size_t total=0;
    for(const auto& widget:items) {
        CHECK(widget.module && names.count(widget.module)==1);
        CHECK(widget.group && std::strlen(widget.group)>0);
        CHECK(widget.label && widget.options && widget.auxiliary);
        CHECK(std::strlen(widget.group)<=70);
        CHECK(std::strlen(widget.label)<=90);
        CHECK(std::strlen(widget.options)<=250);
        CHECK(std::strlen(widget.auxiliary)<=250);
        CHECK(std::isfinite(widget.min_value) &&
              std::isfinite(widget.max_value) &&
              std::isfinite(widget.example_value) &&
              std::isfinite(widget.example_other));
        auto kind=static_cast<unsigned>(widget.kind);
        CHECK(kind<counts.size());
        ++counts[kind];
        if(widget.kind==RecoveredWidgetKind::Slider ||
           widget.kind==RecoveredWidgetKind::Range) {
            CHECK(widget.min_value<widget.max_value);
            CHECK(widget.example_value>=widget.min_value &&
                  widget.example_value<=widget.max_value);
        }
        if(widget.kind==RecoveredWidgetKind::Dropdown ||
           widget.kind==RecoveredWidgetKind::MultiSelect)
            CHECK(std::strlen(widget.options)>0);
        ++total;
    }
    CHECK(total==378);
    CHECK(counts[0]==202 && counts[1]==111 &&
          counts[2]==32 && counts[3]==4 &&
          counts[4]==13 && counts[5]==8 &&
          counts[6]==6 && counts[7]==2);
    std::size_t modulesWithSettings=0;
    std::size_t counted=0;
    for(const auto& m:catalog) {
        const auto count=recovered_widget_count(m.name);
        std::array<const RecoveredWidget*,5> preview{};
        const auto seen=recovered_widget_preview(m.name,preview);
        CHECK(seen==count);
        if(count>0)++modulesWithSettings;
        counted+=count;
        for(std::size_t i=0;i<preview.size();++i) {
            if(i<count) {
                CHECK(preview[i]!=nullptr);
                CHECK(std::string_view(preview[i]->module)==m.name);
            } else CHECK(preview[i]==nullptr);
        }
    }
    CHECK(modulesWithSettings>=55);
    CHECK(counted==378);
    CHECK(recovered_widget_count("Does Not Exist")==0);
    std::array<const RecoveredWidget*,5> empty{};
    CHECK(recovered_widget_preview(nullptr,empty)==0);
    CHECK(empty[0]==nullptr);
    CHECK(recovered_widget_preview("Aim Assist",empty)>=5);
    CHECK(empty[0]->kind==RecoveredWidgetKind::Dropdown);
    CHECK(std::string_view(empty[0]->label)=="Mode");
    CHECK(empty[1]->kind==RecoveredWidgetKind::Slider);
    CHECK(std::string_view(empty[1]->label)=="Horizontal speed");
    CHECK(recovered_widget_preview("Friends",empty)>=4);
    CHECK(empty[0]->kind==RecoveredWidgetKind::Boolean);
    CHECK(empty[1]->kind==RecoveredWidgetKind::Text);
    CHECK(empty[2]->kind==RecoveredWidgetKind::Button);
    CHECK(recovered_widget_preview("Piercing",empty)>=1);
    CHECK(empty[0]->kind==RecoveredWidgetKind::MultiSelect);
    CHECK(recovered_widget_preview("Backtrack",empty)>=5);
    CHECK(recovered_widget_preview("Pointers",empty)>=5);
    // R117: every recovered control must be reachable exactly once through
    // aligned, bounded pages; no reading past module boundaries, and no
    // malformed offset produces partial/stale pointers.
    std::size_t pageVisited=0;
    for(const auto& m:catalog) {
        const auto expected=recovered_widget_count(m.name);
        for(std::size_t offset=0;offset<expected;offset+=5) {
            std::array<const RecoveredWidget*,5> page{};
            CHECK(recovered_widget_page(m.name,offset,page)==expected);
            for(std::size_t i=0;i<page.size();++i) {
                if(offset+i<expected) {
                    CHECK(page[i] && std::string_view(page[i]->module)==m.name);
                    CHECK(std::any_of(items.begin(),items.end(),
                        [&](const RecoveredWidget& entry) {
                            return &entry==page[i];
                        }));
                    ++pageVisited;
                } else CHECK(page[i]==nullptr);
            }
        }
        std::array<const RecoveredWidget*,5> refused{};
        CHECK(recovered_widget_page(m.name,expected+5,refused)==0);
        CHECK(refused[0]==nullptr);
        CHECK(recovered_widget_page(m.name,1,refused)==0);
        CHECK(refused[0]==nullptr);
    }
    CHECK(pageVisited==378);
    std::array<const RecoveredWidget*,5> refused{};
    CHECK(recovered_widget_page(nullptr,0,refused)==0);
    CHECK(recovered_widget_page("Unknown module",0,refused)==0);
    CHECK(recovered_widget_page("Aim Assist",99999,refused)==0);
    CHECK(recovered_widget_page("Aim Assist",5,refused)>5);
    CHECK(std::string_view(refused[0]->label)=="Keep move direction");
    // R120: these three GUI strings are verified against the native
    // registration structures in the pinned, user-provided decrypted .data
    // section, NOT against the estimated mods_block.js fixture.
    const auto native_label=[&](std::string_view module,
                                std::string_view label,
                                RecoveredWidgetKind kind,
                                std::string_view options) {
        const auto it=std::find_if(items.begin(),items.end(),
            [&](const RecoveredWidget& widget) {
                return std::string_view(widget.module)==module &&
                    std::string_view(widget.label)==label &&
                    widget.kind==kind;
            });
        CHECK(it!=items.end());
        CHECK(std::string_view(it->options)==options);
    };
    native_label("Auto Clicker","Click pattern",
        RecoveredWidgetKind::Dropdown,"Jitter|Butterfly");
    // R121: assert every Auto Clicker W_* callback entry is in native order
    // and its independently recovered heading, not the JS fixture order.
    const std::array<const char*,9> nativeAutoNames{{
        "Click pattern","Target CPS","Randomize","Simulate exhaust",
        "Allow breaking blocks","Click in inventory","Randomize speed",
        "Holding weapon","Not using item"
    }};
    const std::array<const char*,9> nativeAutoGroups{{
        "In-game","In-game","In-game","In-game","In-game",
        "Inventory","Inventory","Conditions","Conditions"
    }};
    const std::array<RecoveredWidgetKind,9> nativeAutoKinds{{
        RecoveredWidgetKind::Dropdown,RecoveredWidgetKind::Slider,
        RecoveredWidgetKind::Boolean,RecoveredWidgetKind::Boolean,
        RecoveredWidgetKind::Boolean,RecoveredWidgetKind::Boolean,
        RecoveredWidgetKind::Boolean,RecoveredWidgetKind::Boolean,
        RecoveredWidgetKind::Boolean
    }};
    std::size_t autoIndex=0;
    for(const auto& widget:items) {
        if(std::string_view(widget.module)!="Auto Clicker")continue;
        CHECK(autoIndex<nativeAutoNames.size());
        CHECK(std::string_view(widget.label)==nativeAutoNames[autoIndex]);
        CHECK(std::string_view(widget.group)==nativeAutoGroups[autoIndex]);
        CHECK(widget.kind==nativeAutoKinds[autoIndex]);
        ++autoIndex;
    }
    CHECK(autoIndex==nativeAutoNames.size());
    std::array<const RecoveredWidget*,5> autoSecondPage{};
    CHECK(recovered_widget_page("Auto Clicker",5,autoSecondPage)==9);
    CHECK(autoSecondPage[0] && autoSecondPage[1] &&
          autoSecondPage[2] && autoSecondPage[3] &&
          autoSecondPage[4]==nullptr);
    CHECK(std::string_view(autoSecondPage[1]->label)=="Randomize speed");
    CHECK(std::string_view(autoSecondPage[2]->group)=="Conditions");
    native_label("Piercing","Sort targets by",
        RecoveredWidgetKind::Dropdown,"Hurt time|Health");
    native_label("Anti Bot","Invalid UUID",
        RecoveredWidgetKind::Dropdown,"Off|Stationary only|Always");
    CHECK(std::none_of(items.begin(),items.end(),
        [](const RecoveredWidget& item) {
            return std::string_view(item.module)=="Anti Bot" &&
                std::string_view(item.label)=="Always stationary";
        }));
    return 0;
}
