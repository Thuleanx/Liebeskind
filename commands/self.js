const Discord = require('discord.js');

const axios = require('axios');
const {retrieveToken} = require('../util/interact.js');
const {palete} = require('../appconfig.js');
const {responseFail} = require('./../util/responseFail.js');

var sprintf = require('sprintf-js').sprintf;

const responseSuccess = (data) => {
	var imgPath = (data.pfp?`http:${data.pfp}`:null) || process.env.API_URL + "/img/defaultPFP.png";
	// var imgPath = "https://i.imgur.com/wSTFkRM.png";
	var embed = new Discord.MessageEmbed().setColor(palete[2])
		.setThumbnail(imgPath)
		.setTitle(data.username)
		.addFields(
			{
				name: "General Info", 
				value: `Points accumulated: ${sprintf("%10.4g", data.points_accumulated)}
						Problems solved: ${data.solved}
						Gain rate: ${sprintf("%10.4g", data.gain_rate)}
						Leagues participated in: ${data.league_participation.length}`
			}
		);
	if ("cf_handle" in data && data.cf_rating) {
		embed.addFields(
			{
				name: "Codeforces Info",
				value: `handle: ${data.cf_handle}
						maxRating: ${data.rating}`
			}
		)
	}
	return embed;
}
module.exports = {
	name: 'self',
	description: 'Get info of the current user',
	category: 'User',
	execute(message, args) {
		const {API_URL} = process.env;
		const username = message.author.username + "*" + message.author.discriminator;

		retrieveToken().then(token => {
			axios.get(`${API_URL}/api/event/user/getInfo?username=${username}`, {
				headers: {
					'Authentication': token
				}
			}).then(
				(res) => {
					message.channel.send(responseSuccess(res.data));
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