gsettings_SCHEMAS = io.github.ecotubehq.player.gschema.xml

@GSETTINGS_RULES@

appstream_in_files = io.github.ecotubehq.player.appdata.xml.in
appstream_XML = $(appstream_in_files:.xml.in=.xml)

%.appdata.xml: %.appdata.xml.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

@APPSTREAM_XML_RULES@

dbusservicedir = $(datadir)/dbus-1/services
dbusservice_in_files = io.github.ecotubehq.player.service.in
dbusservice_DATA = $(dbusservice_in_files:.service.in=.service)

%.service: %.service.in Makefile
	$(AM_V_GEN) sed -e "s|\@bindir\@|$(bindir)|" $< > $@

desktopdir = $(datadir)/applications
desktop_in_files = io.github.ecotubehq.player.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

%.desktop: %.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

icondir = $(datadir)/icons/hicolor/scalable/apps
icon_DATA = io.github.ecotubehq.player.svg

symicondir = $(datadir)/icons/hicolor/symbolic/apps
symicon_DATA = io.github.ecotubehq.player-symbolic.svg

EXTRA_DIST =	$(appstream_in_files) \
		$(desktop_in_files) \
		$(dbusservice_in_files) \
		io.github.ecotubehq.player.svg \
		io.github.ecotubehq.player-symbolic.svg \
		io.github.ecotubehq.player.gschema.xml \
		celluloid-mpris-gdbus.xml \
		celluloid-marshal.lst \
		meson.build

DISTCLEANFILES = $(appstream_XML) $(desktop_DATA) $(dbusservice_DATA)
