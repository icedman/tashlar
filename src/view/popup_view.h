#ifndef POPUP_VIEW_H
#define POPUP_VIEW_H

#include "view.h"

#include <memory>

struct popup_root_view_t : view_t
{
	popup_root_view_t();

	static popup_root_view_t* instance();

    bool hasPopups();
	void pushPopup(std::shared_ptr<view_t> p);
	void popPopup();
	void clear();
	bool input(char ch, std::string keys) override;

	std::vector<std::shared_ptr<view_t>> popups;
};

#endif // POPUP_VIEW_H