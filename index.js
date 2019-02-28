var addon = require('bindings')('scanner');

exports.init = function (winHandle) {
    return addon.init(winHandle);
}