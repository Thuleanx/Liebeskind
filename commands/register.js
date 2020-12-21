const axios = require('axios');
const {prefix} = require('../appconfig.js');
const {retrieveToken} = require('../util/interact.js');
const {responseFail} = require('../util/responseFail.js');

const responseSuccess = () => {
	return `Registered! You are in. Try to link your codeforces account using \`${prefix}linkCF [cf_handle]\`!`;
}

module.exports = {
	name: 'register',
	description: `Register the discord user as a participant. After this, they'll be able to link their codeforces, and start participating in leagues.`,
	guildOnly: true,
	category: 'User',
	execute(message, args) {
		const {API_URL} = process.env;
		const username = message.author.username + "*" + message.author.discriminator;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/user/create?username=${username}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(_) => {
					message.channel.send(responseSuccess());
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