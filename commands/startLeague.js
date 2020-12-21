const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('./../util/responseFail.js');
const {prefix} = require('../appconfig.js');

const responseSuccess = (league_name) => {
	return `${league_name} has started. Go solve some problems before it ends. Remember to use \`${prefix}update ${league_name}\` to update your progress.`;
}

module.exports = {
	name: 'startLeague',
	description: 'Start a league. Any user who still have not been assigned to a team will be put into a team.',
	args: 1,
	usage: '<league_name>',
	guildOnly: true,
	adminOnly: true,
	category: 'League',
	execute(message, args) {
		const {API_URL} = process.env;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/league/start?league_name=${args[0]}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(args[0]));
				},
				(err) => { 
					message.channel.send(responseFail(message, err.response.data.comment));
				}
			);
		}, (err) => { 
			message.channel.send(responseFail(message, err.response || `Seems like the server refuses to connect.`)); 
		});
	}
}