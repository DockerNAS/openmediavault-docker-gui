// require("js/omv/WorkspaceManager.js")
// require("js/omv/workspace/panel/Panel.js")
// require("js/omv/module/admin/service/docker/ImageGrid.js")
// require("js/omv/module/admin/service/docker/ContainerGrid.js")

Ext.define("OMV.module.admin.service.docker.Overview", {
	extend: "OMV.workspace.panel.Panel",
	requires: [
		"OMV.module.admin.service.docker.ImageGrid"
	],
	layout: {
		type: 'vbox',
		align: 'stretch'
	},

    initComponent : function() {
        this.on("afterrender", function () {
            var parent = this.up("tabpanel");
			
            if (!parent) {
                return;
            }

            var overviewPanel = parent.down("panel[title=" + _("Overview") + "]");
            var settingsPanel = parent.down("panel[title=" + _("Settings") + "]");
            var checked = settingsPanel.findField("enabled").checked;
			var version = settingsPanel.findField("version").getValue();

            if (overviewPanel) {
                if (checked && version !== "0") {
					overviewPanel.enable();
					overviewPanel.tab.show();
					parent.setActiveTab(overviewPanel);
				} else {
					overviewPanel.disable();
					overviewPanel.tab.hide();
					parent.setActiveTab(settingsPanel);
				}
			}
		}, this);

		this.callParent(arguments);
	},

	items: [{
		xtype: 'module.admin.service.docker.imagegrid',
		flex: 1,
		collapsible: true,
		title: 'Docker Images'
	},{
		xtype: 'splitter'
	},{
		xtype: 'module.admin.service.docker.containergrid',
		flex: 1,
		collapsible: true,
		title: 'Docker Containers'
	}]

});

OMV.WorkspaceManager.registerPanel({
	id: "overview",
	path: "/service/docker",
	text: _("Overview"),
	position: 10,
	className: "OMV.module.admin.service.docker.Overview"
});
